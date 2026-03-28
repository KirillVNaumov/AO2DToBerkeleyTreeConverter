#!/usr/bin/env python3

# ---------------------------------------------
# ------- H Y P E R D O W N L O A D E R -------
# ---------------------------------------------
# This tool is meant to download files from Hyperloop
# Arguments:
# -c / --config: The YAML configuration file
# ---------------------------------------------
# Example usage:
# python download_hyperloop.py --config <path/to/config>

import argparse
import json
import logging
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
from enum import Enum, auto
from glob import iglob
from pathlib import Path

import yaml
from rich.logging import RichHandler
from rich.progress import Progress

log = logging.getLogger('downloader')
log.setLevel(logging.DEBUG)
log.addHandler(RichHandler(level = logging.INFO, log_time_format = "[%X]"))

class Mode(Enum):
    DOWNLOAD = auto()
    RETRY = auto()
    CONFIRM = auto()

# create a filepair class that has source and destination string
class FilePair:
    def __init__(self, src, dst, ntries = 5, timeout = 40):
        self.ntries = ntries
        self.timeout = timeout
        self.src = src
        self.dst = dst

        self.remote = f"alien://{self.src}"
        self.local  = f"file:{self.dst}"

    def __str__(self):
        return f"Source: {self.src}, Destination: {self.dst}"

    def __repr__(self):
        return f"Source: {self.src}, Destination: {self.dst}"

    def download(self):
        log.debug(f"Downloading {self.remote} to {self.local}")
        try:
            subprocess.check_output(f'alien_cp -f -retry {self.ntries} -timeout {self.timeout} {self.remote} {self.local}',
                                    shell=True, encoding='utf-8', stderr = subprocess.PIPE)
        except subprocess.CalledProcessError as e:
            # catch, reparse and raise
            raise FailedDownloadError(e, self.src, self.dst)
        return f"Download success: {self.remote} to {self.local}"

class FailedDownloadError(subprocess.CalledProcessError):
    def __init__(self, e, src, dst):
        super().__init__(e.returncode, e.cmd, e.stdout.strip(), e.stderr.strip())
        self.src = src
        self.dst = dst

class HyperDownloader:
    _defaults = {
        "filename": "AO2D.root",
        "nthreads": 40,
        "ntries": 5,
        "timeout": 150
    }
    def __init__(self, config_file):
        self.check_alien()

        self.configure(config_file)

    def configure(self, config_file):
        cfg = self.get_cfg(config_file)
        self.dataset = cfg["dataset"]

        self.output = f"/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/{self.dataset}/AO2D"

        if not os.path.isdir(self.output) or not os.listdir(self.output):
            os.makedirs(self.output)
            self.mode = Mode.DOWNLOAD
            fh = logging.FileHandler(f'{self.output}/download.log', mode = 'w')
            log.info("Output directory created.")
        elif os.path.isfile(f"{self.output}/download_done"):
            # Check if download is complete already
            self.mode = Mode.CONFIRM
            fh = logging.FileHandler(f'{self.output}/confirm.log', mode = 'w')
            log.info("Output directory already exists.")
            log.info( "Download is already complete. To fully restart, delete the output directory with:")
            log.info(f"  rm -rf {self.output}")
            sys.exit(0)
        elif os.path.isfile(f"{self.output}/retry_paths.txt"):
            self.mode = Mode.RETRY
            fh = logging.FileHandler(f'{self.output}/retry.log', mode = 'w')
            log.info("Output directory already exists.")
        else:
            log.critical("Mode could not be determined, crashing.")
            sys.exit(10)

        fh.setLevel(logging.DEBUG)
        fh.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s', '%Y-%m-%d %H:%M:%S'))
        log.addHandler(fh)

        log.info( "Starting HyperDownloader...")
        log.info(f"Reading HyperDownloader configuration from: {self.config_file}")

        self.train = cfg["download"]["train"]
        self.hyperdirs = cfg["download"]["hyperdirs"].split(",")
        self.filename = cfg["download"].get("filename", self._defaults["filename"])
        self.nthreads = cfg["download"].get("nthreads", self._defaults["nthreads"])
        self.ntries = cfg["download"].get("ntries", self._defaults["ntries"])
        self.timeout = cfg["download"].get("timeout", self._defaults["timeout"])

        log.info( "HyperDownloader configuration:")
        log.info(f"  Mode: {self.mode.name}")
        log.info(f"  Dataset: {self.dataset}")
        log.info(f"  Output directory: {self.output}")
        log.info(f"  Train: {self.train}")
        log.info(f"  Number of Hyperloop directories: {len(self.hyperdirs)}")
        log.info(f"  Filename: {self.filename}")
        log.info(f"  Threads: {self.nthreads}")
        log.info(f"  Tries per file download: {self.ntries}")
        log.info(f"  Download timeout: {self.timeout}")

    def get_cfg(self, config_file):
        if not config_file.endswith(".yaml"):
            config_file = f"{config_file}.yaml"
        base_path = Path(__file__).resolve().parent.parent

        if config_file.startswith("/"):
            self.config_file = config_file
        elif os.path.isfile(f"{base_path}/config/{config_file}"):
            self.config_file = f"{base_path}/config/{config_file}"
        elif os.path.isfile(f"{os.getcwd()}/{config_file}"):
            self.config_file = f"{os.getcwd()}/{config_file}"
        else:
            raise FileNotFoundError("Could not find a valid configuration file!")

        with open(self.config_file) as stream:
            cfg = yaml.safe_load(stream)
        return cfg

    def check_alien(self):
        log.info("Checking AliEn token...")
        # try to run alien_ls and see if it returns nonzero exit code
        result = subprocess.run('alien-token-info', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode == 0:
            # valid token
            log.info("AliEn token validated.")
        elif result.returncode == 2:
            # no valid token
            log.critical("No valid AliEn token found. Run `source get_token.sh` to refresh your token.")
            sys.exit(2)
        elif result.returncode == 127:
            # alien-token-info not found
            log.critical("No valid alien-token-info command: are you in the right alienv?")
            sys.exit(127)
        else:
            log.critical("Unrecognized error, crashing out.")
            sys.exit(result.returncode)

    def download(self):
        # Loop over list of Hyperloop directories and find all matching files inside
        if self.mode is Mode.RETRY:
            log.info("Found retry paths to attempt.")
            aod_paths = self.get_paths_from_file(f"{self.output}/retry_paths.txt")
            log.info(f"Found {len(aod_paths)} remaining AO2D paths to retry.")
        elif self.mode is Mode.CONFIRM:
            log.info("Found cached AO2D paths.")
            aod_paths = self.get_paths_from_file(f"{self.output}/aod_paths.txt")
            log.info(f"Found {len(aod_paths)} AO2Ds.")
        else:
            log.info("Did not find cached AO2D paths.")
            log.info(f"Searching {len(self.hyperdirs)} Hyperloop directories for files named {self.filename}...")
            aod_paths = self.get_aod_paths()
            log.info(f"Found {len(aod_paths)} AO2Ds.")

        # HACK: Find the biggest number of slashes in aod_paths to get only files at lowest level
        max_nslashes = max([path.count('/') for path in aod_paths])
        aod_paths = [x for x in aod_paths if x.count('/') == max_nslashes]
        log.info(f"Removed potential duplicates. Found {len(aod_paths)} files to download.")

        # Set up GRID and local paths
        pairs = []
        for d in aod_paths:
            hy_id = [x for x in d.split('/') if x.startswith('hy_')][0]
            sub_id = d.split('/')[-2]
            local_path = f"{self.output}/{hy_id}/{sub_id}/{d.split('/')[-1]}"
            pairs.append(FilePair(d, local_path, self.ntries, self.timeout))
        nfiles = len(pairs)

        if self.mode is Mode.CONFIRM:
            failed = self.confirm_download(pairs)
            if failed:
                log.error(f"Download confirmation failed on {len(failed)} files.")
                return 4
            log.info(f"Download confirmed for {nfiles} files!")
            log.info( "All done. Good night.")
            return 0

        # if not in CONFIRM mode, download files
        failed = self.download_files(pairs)

        if failed:
            log.warning("Found some files that failed to download, retrying...")
            abandoned = self.download_files(failed)

            if abandoned:
                log.error("The following files could not be downloaded:")
                [log.error(f"  {pair.src} to {pair.dst}") for pair in abandoned]
                abandoned_paths = [pair.src for pair in abandoned]
                self.write_paths_to_file(f"{self.output}/retry_paths.txt", abandoned_paths)
                log.info(f"Grid paths to remaining files have been saved to: {self.output}/retry_paths.txt")
                log.info( "You can reattempt the remaining files by rerunning the downloader with the same command.")
                exit_code = 5
            else:
                log.info("No failed files to retry. Download fully complete!")
                with open(f"{self.output}/download_done", 'w'):
                    pass
                exit_code = 0
        else:
            log.info("No failed files to retry. Download fully complete!")
            with open(f"{self.output}/download_done", 'w'):
                pass
            exit_code = 0

        ao2d_filelist, nfiles = self.create_filelist()
        log.info(f"Download complete: {nfiles} files were downloaded.")

        self.create_readme(ao2d_filelist)

        log.info("All done. Good night.")
        return exit_code

    def download_files(self, pairs):
        nfiles = len(pairs)
        if self.nthreads > nfiles:
            log.warning(f"More threads than files, reducing to {nfiles}.")
            self.nthreads = nfiles
        log.info(f"Starting download of {nfiles} files with {self.nthreads} threads.")
        failed = []
        with ThreadPoolExecutor(max_workers = self.nthreads) as executor, Progress() as progress:
            task = progress.add_task("[green]Downloading files...", total = nfiles, refresh_per_second = 1)
            futures = [executor.submit(pair.download) for pair in pairs]

            for future in as_completed(futures):
                try:
                    msg = future.result()
                    log.debug(msg)
                except FailedDownloadError as e:
                    log.error(f"Failed download with exit code {e.returncode} after {self.ntries} attempts:")
                    log.error(e.stdout)
                    log.error(e.stderr)
                    failed.append(FilePair(e.src, e.dst, self.ntries, self.timeout))
                progress.update(task, advance=1)

        nfailed = len(failed)
        log.info(f"Completed download: {nfiles - nfailed} successful, {nfailed} failed.")
        return failed

    def confirm_download(self, pairs):
        nfiles = len(pairs)
        if self.nthreads > nfiles:
            log.warning(f"More threads than files, reducing to {nfiles}.")
            self.nthreads = nfiles
        log.info(f"Confirming download of {nfiles} files with {self.nthreads} threads.")
        failed = []
        with ThreadPoolExecutor(max_workers = self.nthreads) as executor, Progress() as progress:
            task = progress.add_task("[green]Confirming file download...", total = nfiles, refresh_per_second = 1)
            futures = [executor.submit(pair.download) for pair in pairs]

            for future in as_completed(futures):
                try:
                    msg = future.result()
                    log.debug(msg)
                except FailedDownloadError as e:
                    log.error(f"Failed confirmation with exit code {e.returncode} after {self.ntries} attempts:")
                    log.error(e.stdout)
                    log.error(e.stderr)
                    failed.append(FilePair(e.src, e.dst, self.ntries, self.timeout))
                progress.update(task, advance=1)

        nfailed = len(failed)
        log.info(f"Completed download confirmation: {nfiles - nfailed} successful, {nfailed} failed.")
        return failed

    def get_paths_from_file(self, filename):
        with open(filename, 'r') as f:
            paths = f.read().strip().splitlines()
            # ignore empty lines
        paths = [path for path in paths if path]
        return paths

    def write_paths_to_file(self, filename, paths):
        all_paths = "\n".join(paths)
        with open(filename, 'w') as f:
            f.write(all_paths)

    def get_aod_paths(self):
        aod_paths = []
        with Progress() as progress:
            search_task = progress.add_task("[green]Searching for files...", total=len(self.hyperdirs),refresh_per_second=1)
            for hydir in self.hyperdirs:
                try:
                    # Use strip() to remove spurious newline at end of alien_find output
                    aod_paths += subprocess.run(f'alien_find {hydir} {self.filename}', shell = True, encoding = 'utf-8',
                                                stdout = subprocess.PIPE).stdout.strip().split('\n')
                except subprocess.CalledProcessError as e:
                    log.error(f"Failed to find {self.filename} in {hydir}: {e}")
                progress.update(search_task, advance=1)
        self.write_paths_to_file(f"{self.output}/aod_paths.txt", aod_paths)
        log.info(f"Search complete. Found {len(aod_paths)} files.")

        return aod_paths

    # def build_retry_filelist(self, failed):
    #     with open(f"{self.output}/redownload.sh", 'w') as f:
    #         f.write("#!/usr/bin/bash\n")
    #         for pair in failed:
    #             f.write(f'alien_cp -f -retry {self.ntries} -timeout {self.timeout} {pair.src} {pair.dst}\n')
    #     log.info( "A script redownload.sh has been prepared. You can attempt another download of the remaining files by running:")
    #     log.info(f"  shifter --module=cvmfs --image=tch285/o2alma:latest /cvmfs/alice.cern.ch/bin/alienv setenv xjalienfs/1.6.9-1 -c bash {self.output}/redownload.sh")
    #     log.info( "Don't forget to recreate the filelist once more files have been downloaded!")
    #     log.info( "You can do this with the command:")
    #     log.info(f"  find {self.output} -type f -name */{self.filename} > {self.output}/filelist2.txt")

    def create_filelist(self):
        """Save filelist of all files with a matching filename."""
        filelist = f'{self.output}/filelist.txt'
        pattern = f"{self.output}/**/{self.filename}"
        nfiles = 0
        with open(filelist, 'w') as f:
            for path in iglob(pattern, recursive = True):
                print(path, file = f)
                nfiles += 1

        log.info(f"Created filelist: {filelist}")
        return filelist, nfiles

    def create_readme(self, ao2d_filelist):
        """Create README with relevant information on the converter and dataset."""

        readme_path = f"{self.output}/README.md"
        if os.path.isfile(readme_path):
            log.info(f"README already present: {readme_path}")
            return
        hash_long       = subprocess.check_output(['git', 'rev-parse', 'HEAD'], encoding='ascii').strip()
        hash_short      = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], encoding='ascii').strip()
        describe        = subprocess.check_output(['git', 'describe'], encoding='ascii').strip()
        branch          = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], encoding='ascii').strip()
        remote, rbranch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', r'@{u}'], encoding='ascii').strip().split("/")
        ssh_url         = subprocess.check_output(['git', 'ls-remote', '--get-url', remote], encoding='ascii').strip()
        http_url        = f"https://github.com/{ssh_url.split(':', 1)[1]}"
        msg             = subprocess.check_output(['git', 'log', '-1', '--pretty=%B'], encoding='ascii').strip()
        dt              = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        o2physics_ver   = self.get_o2physics_version()
        dirlist         = '\n'.join([f"- {dirname}" for dirname in self.hyperdirs])

        readme = (  f"# HYPERDOWNLOADER\n\n"
                    f"Dataset downloaded on {dt}\n\n"
                    "## Converter\n\n"
                    f"- Version: {describe}\n"
                    f"- Branch: '{branch}'\n"
                    f"- Tracking branch '{rbranch}' on remote {http_url}\n"
                    f"- Commit hash (short / long): {hash_short} / {hash_long}\n"
                    f"  - {msg}\n\n"
                    f"## Configuration\n\n"
                    f"- Mode: {self.mode.name}\n"
                    f"- Dataset: {self.dataset}\n"
                    f"- Train: {self.train}\n"
                    f"- Filename: {self.filename}\n"
                    f"- Number of threads: {self.nthreads}\n"
                    f"- Number of tries per file download: {self.ntries}\n"
                    f"- Timeout: {self.timeout}\n"
                    f"- Output directory: {self.output}\n\n"
                    f"## Dataset\n\n"
                    f"O2Physics version:\n"
                    f"  {o2physics_ver}\n\n"
                    "Hyperloop directories:\n\n"
                    f"{dirlist}\n\n"
                    "Filelist:\n\n"
                    f"- {ao2d_filelist}\n"
        )

        with open(readme_path, 'w') as f:
            f.write(readme)
        log.info(f"README created: {readme_path}")

    def get_o2physics_version(self, json_file = "full_config.json"):
        try:
            train_prefix = f"{self.train // 10000:04d}" # cut off last four digits and pad with zeroes on left side
            search_path = f"/alice/cern.ch/user/a/alihyperloop/outputs/{train_prefix}/{self.train}"
            cmd = subprocess.run(f'alien_find {search_path} {json_file}', shell = True, encoding = 'utf-8',
                                    stdout = subprocess.PIPE, stderr = subprocess.PIPE)
            path = cmd.stdout.strip()
            if cmd.returncode == 0:
                if not path:
                    # alien_find returns 0 even if no file was found
                    log.error(f"Train config '{json_file}' could not be located in: {search_path}")
                    return "TRAIN CONFIG NOT FOUND"
            else:
                log.error(f"JSON finding failed with error {cmd.returncode}: {cmd.stderr.strip()}")
                return "TRAIN DIRECTORY NOT FOUND"
            pair = FilePair(path, f"{self.output}/", self.ntries, self.timeout)
            msg = pair.download()
            log.debug(msg)
            with open(f'{self.output}/{json_file}', 'r') as file:
                cfg = json.load(file)
            version = cfg["package_tag"]
            log.info(f"Found O2Physics version: {version}")
            return version
        except subprocess.CalledProcessError as e:
            log.error(f"Failed to find config in {search_path}: {e}")
        except FailedDownloadError as e:
            log.error(f"Failed download with exit code {e.returncode} after {self.ntries} attempts:")
            log.error(e.stdout)
            log.error(e.stderr)
        msg =  "Failed to find/download train configuration JSON."
        log.error(msg)
        return "NOT FOUND"

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Download a list of files', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-c', '--config', help='Path to the config YAML file.')
    args = parser.parse_args()

    downloader = HyperDownloader(args.config)
    sys.exit(downloader.download())