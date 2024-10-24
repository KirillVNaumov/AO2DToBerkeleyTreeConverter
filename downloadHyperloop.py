# ---------------------------------------------
# ------- H Y P E R D O W N L O A D E R -------
# ---------------------------------------------
# This tool is meant to download files from hyperloop
# Arguments:
# --inputfilelist: The txt file containing the list of files to download. They should be comma separated list that one can get by clicking on a train -> Submitted jobs -> Copy all output directories
# --outputfolder: The folder to save the downloaded files
# --filename: The name of the downloaded file e.g. AO2D.root or comma separated list of files if you want to download multiple files
# --isDerived: If the data to download is derived data or not (important for the path)
# --nThreads: Number of threads to use for downloading
# ---------------------------------------------
# Example usage:
# python downloadHyperloop.py --inputfilelist=/path/to/inputfilelist.txt --outputfolder=/path/to/outputfolder --filename=AO2D.root --isDerived=True --nThreads 4

import sys
import os
import argparse
import subprocess
import threading
import logging
import time
from pathlib import Path
from rich.logging import RichHandler
from rich.progress import Progress, BarColumn, TextColumn, TimeRemainingColumn, TimeElapsedColumn, SpinnerColumn, MofNCompleteColumn

# setup logging
logging.basicConfig(
    level=logging.INFO,
    format="%(message)s",
    datefmt="[%X]",
    handlers=[RichHandler()]
)

log = logging.getLogger("rich")


# create a filepair class that has source and desitnation string
class FilePair:
    def __init__(self, source, destination):
        self.source = source
        self.destination = destination

    def __str__(self):
        return f"Source: {self.source}, Destination: {self.destination}"

    def __repr__(self):
        return f"Source: {self.source}, Destination: {self.destination}"

def downloadAlien(downloadpairs,task,progress,verbose=False):
    for downloadpair in downloadpairs:
        alienpath = downloadpair.source
        localpath = downloadpair.destination
        if verbose:
            log.info(f"Downloading {alienpath} to {localpath}")
        # try to catch an error, print it and continue
        try:
            subprocess.check_output(f'alien_cp {alienpath} {localpath}', shell=True)
        except subprocess.CalledProcessError as e:
            log.error(f"Failed to download {alienpath} to {localpath}")
            log.error(e)
            return 0
        progress.update(task, advance=1)
    return 1
    
        
        
def checkAlien():
    # try to run alien_find and see if it returns "command not found"
    out = ""
    out = subprocess.run('alien_find', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if "command not found" in out.stderr.decode('utf-8'):
        return False
    else:
        return True
def createFileList(folder, filename):
    # create a new txt file in folder called fullInputList_<filename>.txt
    filenameNoEnding = filename.split('.')[0]
    with open(f'{folder}/fullInputList_{filenameNoEnding}.txt', 'w') as f:
        f.write(filename)
    log.info(f"Created file {folder}/fullInputList_{filenameNoEnding}.txt")
    # find all files with filename in folder and write them to the file one line each
    files = subprocess.check_output(f'find {folder} -name {filename}', shell=True).decode('utf-8').split('\n')
    with open(f'{folder}/fullInputList_{filenameNoEnding}.txt', 'w') as f:
        for file in files:
            f.write(file + '\n')
    

def downloadHyperloop(inputfilelist, outputfolder, filename):
    # create output folder if it doesn't exist
    if not os.path.exists(outputfolder):
        os.makedirs(outputfolder)
        log.info(f"Created folder {outputfolder}")
    else:
        log.warning(f"Folder {outputfolder} already exists.")

    # check if the output folder exists
    inputpaths = []
    inputfiles = []
    # if filename is comma separated list, split it
    if ',' in filename:
        inputfiles=filename.split(',')
    else:
        inputfiles.append(filename)
    with open(inputfilelist, 'r') as f:
        inputpaths = f.read().split(',')

    
    # loop over list of inputpaths and print them
    downloadpaths = []
    hasAODFolder = True
    log.info(f"Checking if there is an /AOD folder in the path ...")
    try:
        checkPathOut = subprocess.check_output(f'alien_find {path}/AOD {file}', shell=True).decode('utf-8').split('\n')
        log.info(f"Found /AOD folder in the path.")
    except:
        checkPathOut = []
        hasAODFolder = False
        log.info(f"No /AOD folder found in the path.")

    totDownloads = len(inputpaths)*len(inputfiles)
    log.info(f"Searching for {totDownloads} directories for files to download...")
    with Progress() as progress:
        search_task = progress.add_task("[green]Searching for files...", total=totDownloads,refresh_per_second=1)
        for path in inputpaths:
            # run bash command and store output
            for file in inputfiles:
                if file == "":
                    continue
                if hasAODFolder:
                    path = f"{path}/AOD"
                try:
                    downloadpaths += subprocess.check_output(f'alien_find {path} {file}', shell=True).decode('utf-8').split('\n')
                except subprocess.CalledProcessError as e:
                    log.error(f"Failed to find {file} in {path}")
                    log.error(e)
                progress.update(search_task, advance=1)

    # find the biggest number of slashes in the downloadpaths. This we do as a dirty hack to really only get those files that are in the lowest level
    number_of_slashes = 0
    for path in downloadpaths:
        if path == "":
            continue
        if path.count('/') > number_of_slashes:
            number_of_slashes = path.count('/')
    # remove everything that has less slashes than the biggest number of slashes
    log.info(f"Search complete. Found {len(downloadpaths)} files to download.")
    downloadpaths = [x for x in downloadpaths if x.count('/') == number_of_slashes]
    log.info(f"Removed potential dublicates. Found {len(downloadpaths)} files to download.")
   
    # do multithreading download
    counter=0
    downloadFilePairs= list()
    for d in downloadpaths:
        if d == "":
            continue
        hyID = [x for x in d.split('/') if x.startswith('hy_')][0]
        subId = d.split('/')[-2]
        localpath = f"file:{outputfolder}/{hyID}/{subId}/{d.split('/')[-1]}"
        alienpath=f"alien://"+d
        # append a pair to the list
        downloadFilePairs.append(FilePair(alienpath, localpath))
        counter+=1



    file_pairs_chunks = [downloadFilePairs[i::args.nThreads] for i in range(args.nThreads)]

    # tasks = []
    # import concurrent.futures
    # with concurrent.futures.ProcessPoolExecutor(max_workers=args.nThreads) as executor:
    #     futures = []
    #     with Progress() as progress:
    #         threadNum = 0
    #         for fps in file_pairs_chunks:
    #             # task_id = progress.add_task(f"[green] Downloading files (thread ) ...", total=len(downloadpaths),refresh_per_second=1)
    #             task_id = progress.add_task(f"[green] Downloading files (thread {threadNum}) ...", total=len(fps),refresh_per_second=1)
    #             futures.append(executor.submit(downloadAlien,fps, task_id, progress, False))
    #             threadNum+=1
    #         for future in concurrent.futures.as_completed(futures):
    #             if future.result() == 1:
    #                 log.info("Download complete.")
    #             else:
    #                 log.error("Download failed.")
    # rewrite the above code using threading
    threads = []
    with Progress() as progress:
        for i in range(args.nThreads):
            task_id = progress.add_task(f"[green] Downloading files (thread {i}) ...", total=len(file_pairs_chunks[i]),refresh_per_second=1)
            t = threading.Thread(target=downloadAlien, args=(file_pairs_chunks[i], task_id, progress, False))
            threads.append(t)
            t.start()
        for t in threads:
            t.join()
        
            
    log.info(f"Download complete. I downloaded {len(downloadpaths)} files.")
    for file in inputfiles:
        if file == "":
            continue
        log.info(f"Creating file list for {file} for easy use in analysis.")
        createFileList(outputfolder, file)

    log.info("All done. Good Night.")
    

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Download a list of files')
    parser.add_argument('--inputfilelist', help='The txt file containing the list of files to download. They should be comma separated', type=str)
    parser.add_argument('--outputfolder', help='The folder to save the downloaded files', type=str)
    parser.add_argument('--filename', help='The name of the downloaded file e.g. AO2D.root or comma separated list of files', type=str)
    parser.add_argument('--nThreads', help='Number of threads to use for downloading', type=int, default=1)

    args = parser.parse_args()
    log.info("Starting HyperDownloader...")
    log.info(f"Inputfilelist: {args.inputfilelist}")
    log.info(f"Outputfolder: {args.outputfolder}")
    log.info(f"Filename(s): {args.filename}")
    log.info(f"nThreads: {args.nThreads}")

    log.info(f"Checking if alien_find is available...")
    if not checkAlien():
        log.error("alien_find not found. Please make sure AliEn is installed and in your PATH.")
        sys.exit(1)
    else:
        log.info("AliEn found.")
    downloadHyperloop(args.inputfilelist, args.outputfolder, args.filename)