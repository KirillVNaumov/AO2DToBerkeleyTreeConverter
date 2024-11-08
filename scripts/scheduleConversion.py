import sys
import os
import argparse
import subprocess
import threading
import logging
import time
from pathlib import Path
from rich.logging import RichHandler
import glob

# setup logging
logging.basicConfig(
    level=logging.INFO,
    format="%(message)s",
    datefmt="[%X]",
    handlers=[RichHandler()]
)

log = logging.getLogger("rich")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Converts a directory of files to a different format")
    parser.add_argument("--converterPath", help="The absolute path to the converter")
    parser.add_argument("--input", help="The input directory")
    parser.add_argument("--output", help="The output directory. If empty, then inputdir/converted will be used", default="")
    parser.add_argument("--filename", help="The input filename", default="AO2D.root")
    parser.add_argument("--nFilesPerJob", help="The number of files to convert per job", default=1)
    parser.add_argument("--config", help="The config file to use", default="treeCuts.yaml")
    args = parser.parse_args()

    if not args.input:
      log.error(f"Input directory was not provided")
      sys.exit(1)

    if not args.converterPath:
      log.error(f"The path to the converter is not provided")
      sys.exit(1)

    converter_binary = Path(args.converterPath + "/converter")
    input_dir = Path(args.input)
    output_dir = args.output
    configfile = args.config

    # check if converter executable is specified
    if not converter_binary.exists() or not converter_binary.is_file():
        log.error(f"Converter binary {converter_binary} does not exist or not a file")
        sys.exit(1)

    # check if input dir is specified
    if not input_dir.exists():
        log.error(f"Input directory {input_dir} does not exist")
        sys.exit(1)
    log.info(f"Using input directory {input_dir}")
    
    # if args.output is empty, then use input_dir/converted
    if output_dir == "":
        log.info(f"No output directory specified. Using {input_dir / 'converted'}")
        output_dir = input_dir / "converted"
    output_dir = Path(output_dir)
   
    # create output directory if it does not exist
    if not output_dir.exists():
        log.info(f"Creating output directory {output_dir}")
        os.makedirs(output_dir)

    # find recursively all files in the input directory
    files = glob.glob(str(input_dir / "**" / args.filename), recursive=True)
    # make sure none of the paths contains the word "converted"
    files = [f for f in files if "converted" not in f]    # print all files

    # remove all files that are not called AO2D.root
    files = [f for f in files if args.filename in f]

    log.info(f"Found {len(files)} files to convert")

    # split the files into chunks each containing nFilesPerJob
    nFilesPerJob = int(args.nFilesPerJob)
    filechunks = [files[i:i + nFilesPerJob] for i in range(0, len(files), nFilesPerJob)]
    log.info(f"Splitting files into {len(filechunks)} chunks with {nFilesPerJob} files each")

    # create a tmp directory to store the tmp files
    tmpdir = input_dir / "tmp"

    # remove the tmp directory if it exists
    if tmpdir.exists():
        os.system(f"rm -rf {tmpdir}")
    log.info(f"Creating tmp directory {tmpdir}")
    os.makedirs(tmpdir)
    # write each chunk to a tmp file and submit a job
    slurmJobIDs = []
    for i, filechunk in enumerate(filechunks):
        # create outputfolder in output_dir
        outputfolder = output_dir / f"{i}"
        if not outputfolder.exists():
            os.makedirs(outputfolder)


        tmpfile = tmpdir / f"tmpfile_{i}.txt"
        with open(tmpfile, "w") as f:
            for file in filechunk:
                f.write(file + "\n")

        tmpscript = tmpdir / f"RunJob_{i}.sh" 
        with open(tmpscript, "w") as f:
            f.write("#!/bin/bash\n")
            f.write(f"inputfilelist={tmpfile}\n")
            f.write(f"outputfile={outputfolder / 'AOD.root'}\n")
            f.write(f"configfile={configfile}\n")
            f.write(f"converter_binary={converter_binary}\n")
            f.write("$converter_binary -i $inputfilelist -o $outputfile -c $configfile \n")
        
        log.info(f"Writing chunk {i} to {tmpfile}")
        # submit a job
        log.info(f"Submitting job for chunk {i}")
        # submit a job
        cmd=f"sbatch {tmpscript}" 
        # save the job id and run
        slurJobID = subprocess.check_output(cmd, shell=True).decode('utf-8').split()[-1]
        log.info(f"Chunk {i} is being processed by {slurJobID}")
        slurmJobIDs.append(slurJobID)


    filelistmakerscript = tmpdir / "filelistmaker.sh"
    with open(filelistmakerscript, "w") as f:
        f.write("#!/bin/bash\n")
        f.write(f"outputdir={output_dir}\n")
        f.write(f"find {output_dir} -name \"AOD.root\" > $outputdir/InputFiles.txt\n")    


    # run sbatch with filelistmakerscript after all jobs from slurJobIDs are done
    cmd = f"sbatch --dependency=afterok:{':'.join(slurmJobIDs)} {filelistmakerscript}"
    log.info(f"Running {cmd}")
    os.system(cmd)
