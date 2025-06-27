#!/usr/bin/env python3

# ---------------------------------------------
# ------- H Y P E R D O W N L O A D E R -------
# ---------------------------------------------
# This tool is meant to download files from Hyperloop
# Arguments:
# --input: The txt file containing the list of files to download. They should be comma separated list that one can get by clicking on a train -> Submitted jobs -> Copy all output directories
# --output: The directory to save the downloaded files
# --filename: The name of the downloaded file e.g. AO2D.root or comma-separated list of files if you want to download multiple files
# --nthreads: Number of threads to use for downloading
# ---------------------------------------------
# Example usage:
# python download_hyperloop.py --input /path/to/inputfilelist.txt --output /path/to/output --filename <name> --nthreads 4

import sys
import os
import shutil
import argparse
import subprocess
import logging
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
from glob import iglob
from rich.logging import RichHandler
from rich.progress import Progress


log = logging.getLogger('downloader')
log.setLevel(logging.DEBUG)
log.addHandler(RichHandler(level = logging.INFO, log_time_format = "[%X]"))

# create a filepair class that has source and destination string
class FilePair:
    def __init__(self, src, dst):
        self.src = src
        self.dst = dst

    def __str__(self):
        return f"Source: {self.src}, Destination: {self.dst}"

    def __repr__(self):
        return f"Source: {self.src}, Destination: {self.dst}"

class FailedDownloadError(subprocess.CalledProcessError):
    def __init__(self, e, src, dst):
        super().__init__(e.returncode, e.cmd, e.stdout.strip(), e.stderr.strip())
        self.src = src
        self.dst = dst

def download_from_alien(pair, ntries):
    alienpath = pair.src
    localpath = pair.dst
    log.debug(f"Downloading {alienpath} to {localpath}")
    try:
        subprocess.check_output(f'alien_cp -f -retry {ntries} {alienpath} {localpath}', shell=True,
                                encoding='utf-8', stderr = subprocess.PIPE)
    except subprocess.CalledProcessError as e:
        # catch error, reparse and raise
        raise FailedDownloadError(e, alienpath, localpath)
    return f"Download success: {alienpath} to {localpath}"

def check_alien():
    log.info("Checking AliEn token...")
    # try to run alien_ls and see if it returns nonzero exit code
    result = subprocess.run('alien-token-info', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0:
        # valid token
        log.info("AliEn token validated.")
    elif result.returncode == 2:
        # no valid token
        log.warning("Found no valid AliEn token.")
        result = subprocess.run('alien-token-init', shell=True, check = True, stderr=subprocess.PIPE)
        log.info("AliEn token created.")
    elif result.returncode == 127:
        # alien-token-info not found
        log.critical("No valid alien-token-info command: are you in the right alienv?")
        sys.exit(127)
    else:
        log.critical("Unrecognized error, crashing out.")
        sys.exit(result.returncode)

def create_filelist(output_dir, filename):
    """Save filelist of all files matching filename to output_dir."""
    clean_name = filename.replace('.', '_')
    filelist = f'{output_dir}/filelist_{clean_name}.txt'
    pattern = f"{output_dir}/**/{filename}"
    nfiles = 0
    with open(filelist, 'w') as f:
        for path in iglob(pattern, recursive = True):
            print(path, file = f)
            nfiles += 1

    log.info(f"Created filelist: {filelist}")
    return filelist, nfiles

def check_output_directory(path):
    if os.path.isdir(path):
        if os.listdir(path):
            log.info("Non-empty AO2D directory found, clearing.")
            # move the download log into parent directory temporarily to avoid deleting it
            shutil.move(f"{path}/download.log", f"{path}/../download.log")
            shutil.rmtree(path)
            os.makedirs(path)
            shutil.move(f"{path}/../download.log", f"{path}/download.log")
            log.info("AO2D directory cleared.")
        else:
            log.info("Output directory found.")
    else:
        os.makedirs(path)
        log.info("Output directory created.")

def create_readme(hyperloop_dirs, filelists, output_dir):
    """Create README with relevant information on the converter and dataset."""

    rm = f"{output_dir}/README.md"
    hash_long = subprocess.check_output(['git', 'rev-parse', 'HEAD'], encoding='ascii').strip()
    hash_short = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], encoding='ascii').strip()
    describe = subprocess.check_output(['git', 'describe'], encoding='ascii').strip()
    branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], encoding='ascii').strip()
    remote, rbranch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', r'@{u}'], encoding='ascii').strip().split("/")
    ssh_url = subprocess.check_output(['git', 'ls-remote', '--get-url', remote], encoding='ascii').strip()
    http_url = f"https://github.com/{ssh_url.split(':', 1)[1]}"
    msg = subprocess.check_output(['git', 'log', '-1', '--pretty=%B'], encoding='ascii').strip()
    dt = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    dirs = '\n'.join([f"- {dirname}" for dirname in hyperloop_dirs])
    flist = '\n'.join([f"- {filelist}" for filelist in filelists])

    readme = (  f"# HYPERDOWNLOADER\n\n"
                f"Dataset downloaded on {dt}\n\n"
                 "## Converter\n\n"
                f"- Version: {describe}\n"
                f"- Branch: '{branch}'\n"
                f"- Tracking branch '{rbranch}' on remote {http_url}\n"
                f"- Commit hash (short / long): {hash_short} / {hash_long}\n"
                f"  - {msg}\n\n"
                f"## Dataset\n\n"
                 "Hyperloop directories:\n\n"
                f"{dirs}\n\n"
                 "Filelists:\n\n"
                f"{flist}\n"
    )
    with open(rm, 'w') as f:
        f.write(readme)
    log.info(f"README created: {rm}")


def download_hyperloop(hyperloop_list, output_dir, filename, nthreads, ntries):
    # if filename is a comma-separated list, split it
    # NOTE: works even if filename has no commas
    input_filenames = [f for f in filename.split(',') if f != ""]

    # The Hyperloop page output has a final comma if there is only one directory,
    # so we remove a possible final comma with strip before splitting.
    with open(hyperloop_list, 'r') as f:
        hyperloop_dirs = f.read().strip(',').split(',')

    # Check if hyperloop directories have AOD subdirectories in them
    log.info("Checking if there is an /AOD directory in the path ...")
    try:
        subprocess.run(f'alien_ls {hyperloop_dirs[0]}/AOD', shell = True, encoding = 'utf-8',
                        check = True, stdout = subprocess.DEVNULL)
        log.info("Found /AOD directory in the path.")
        end = ""
    except subprocess.CalledProcessError:
        log.info("No /AOD directory found in the path.")
        end = "/AOD"

    # loop over list of hyperloop_dirs and find all matching files inside
    log.info(f"Searching {len(hyperloop_dirs)} Hyperloop directories for {len(input_filenames)} filename(s)...")
    tot_downloads = len(hyperloop_dirs) * len(input_filenames)
    aod_paths = []
    with Progress() as progress:
        search_task = progress.add_task("[green]Searching for files...", total=tot_downloads,refresh_per_second=1)
        for hydir, filename in ((hd, fn) for hd in hyperloop_dirs for fn in input_filenames):
            try:
                # Use strip() to remove spurious newline at end of alien_find output
                aod_paths += subprocess.run(f'alien_find {hydir}{end} {filename}', shell = True, encoding = 'utf-8',
                                            stdout = subprocess.PIPE).stdout.strip().split('\n')
            except subprocess.CalledProcessError as e:
                log.error(f"Failed to find {filename} in {hydir}: {e}")
            progress.update(search_task, advance=1)
    log.info(f"Search complete. Found {len(aod_paths)} files.")

    # HACK: Find the biggest number of slashes in aod_paths to get only files at lowest level
    max_nslashes = max([path.count('/') for path in aod_paths])
    aod_paths = [x for x in aod_paths if x.count('/') == max_nslashes]
    log.info(f"Removed potential duplicates. Found {len(aod_paths)} files to download.")
   
    # Do multithreading download
    pairs = []
    for d in aod_paths:
        hy_id = [x for x in d.split('/') if x.startswith('hy_')][0]
        sub_id = d.split('/')[-2]
        local_path = f"file:{output_dir}/{hy_id}/{sub_id}/{d.split('/')[-1]}"
        alien_path = f"alien://{d}"
        pairs.append(FilePair(alien_path, local_path))
    nfiles = len(pairs)

    # Reduce number of threads if we have more threads than files
    if nthreads > nfiles:
        nthreads = nfiles
        log.warning(f"More threads than files, reducing to {nfiles}.")

    failed = []
    with ThreadPoolExecutor(max_workers = nthreads) as executor, Progress() as progress:
        task = progress.add_task("[green]Downloading files...", total = nfiles, refresh_per_second = 1)
        futures = [executor.submit(download_from_alien, pair, ntries) for pair in pairs]

        for future in as_completed(futures):
            try:
                msg = future.result()
                log.debug(msg)
            except FailedDownloadError as e:
                log.error(f"Failed download with exit code {e.returncode} after {ntries} attempts:")
                log.error(e.stdout)
                log.error(e.stderr)
                failed.append(FilePair(e.src, e.dst))
            progress.update(task, advance=1)

    nfailed = len(failed)
    log.info(f"Completed download: {nfiles - nfailed} successful, {nfailed} failed.")

    abandoned = []
    if failed:
        log.warning("Found some files failed to download, retrying...")
        with ThreadPoolExecutor(max_workers = nthreads) as executor, Progress() as progress:
            task = progress.add_task("[green]Redownloading files...", total = nfailed, refresh_per_second = 1)
            futures = [executor.submit(download_from_alien, pair, ntries) for pair in failed]

            for future in as_completed(futures):
                try:
                    msg = future.result()
                    log.debug(msg)
                except FailedDownloadError as e:
                    log.error(f"Failed redownload with exit code {e.returncode} after {ntries} attempts:")
                    log.error(e.stdout)
                    log.error(e.stderr)
                    abandoned.append(FilePair(e.src, e.dst))
                progress.update(task, advance=1)

        nabandoned = len(abandoned)
        log.info(f"Completed redownload: {nfailed - nabandoned} successful, {nabandoned} abandoned.")
        if abandoned:
            log.error("The following files could not be downloaded:")
            [log.error(f"  {pair.src} to {pair.dst}") for pair in abandoned]
    else:
        log.info("No failed files to retry.") 

    tot_files = 0
    info = [create_filelist(output_dir, filename) for filename in input_filenames]
    filelists = [i[0] for i in info]
    tot_files = sum([i[1] for i in info])
    log.info(f"Download complete: {tot_files} files were downloaded.")

    create_readme(hyperloop_dirs, filelists, output_dir)

    log.info("All done. Good night.")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Download a list of files', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-i', '--input',      help='The single-line txt file containing a comma-delimited list of grid files to download.')
    parser.add_argument('-o', '--output',     help='The directory in which to save the downloaded files')
    parser.add_argument('-f', '--filename',   help='The name of the downloaded file e.g. AO2D.root or comma separated list of files', default = "AO2D.root")
    parser.add_argument('-n', '--nthreads',   help='Number of threads to use for downloading', type = int, default = 10)
    parser.add_argument('-t', '--ntries',     help='Number of retries for downloading', type = int, default = 5)
    args = parser.parse_args()

    # check if the output directory exists and create if it doesn't
    # need this early to set up the FileHandler properly
    check_output_directory(args.output)

    fh = logging.FileHandler(f'{args.output}/download.log', mode = 'w')
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
    log.addHandler(fh)

    log.info( "Starting HyperDownloader...")
    log.info(f"Input filelist: {args.input}")
    log.info(f"Output directory: {args.output}")
    log.info(f"Filename(s): {args.filename}")
    log.info(f"Number of threads: {args.nthreads}")
    log.info(f"Number of tries: {args.ntries}")

    check_alien()
    download_hyperloop(args.input, args.output, args.filename, args.nthreads, args.ntries)