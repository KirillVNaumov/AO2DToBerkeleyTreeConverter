# Scripts directory

This directory contains all scripts that are required to perform the AO2D to BerkeleyTree conversion.

## launchPipeline.sh

This is the main script that compiles the converter, prepares the AO2D.root filesm and runs/schedules the conversion. 

### Requirements
`sbatch`, Alice utils (specifically, `alien_find`), `rich` Python module
### Arguments
```
HYPERLOOP_TXT: Path to the list containing Hyperloop files to download (default: "../hyperloop_list.txt")
HYPERLOOP_DATA_DIR: Path to the directory with the initial AO2D files downloaded from the Hyperloop (default: "../AO2Ds")
OUTPUT_DIR: Path to the converted files (default: "../BerkeleyTrees")
```
### How to run
Once the above parameters are configured for your setup, simply run `bash launchPipeline.sh`


## downloadHyperloop.py
This is the script used to download the AO2D files from the Hyperloop
### Requirements
Alice utils (specifically, `alien_find`), `rich` Python module

### Arguments
```
  -h, --help            show this help message and exit
  --inputfilelist INPUTFILELIST
                        The txt file containing the list of files to download. They should be comma separated
  --outputfolder OUTPUTFOLDER
                        The folder to save the downloaded files
  --filename FILENAME   The name of the downloaded file e.g. AO2D.root or comma separated list of files
  --nThreads NTHREADS   Number of threads to use for downloading
```

### How to run
See an example in launchPipeline.sh

## scheduleConversion.py
### Requirements
`sbatch`, `rich` Python module

### Arguments
```
  -h, --help            show this help message and exit
  --converterPath CONVERTERPATH
                        The absolute path to the converter
  --input INPUT         The input directory
  --output OUTPUT       The output directory. If empty, then inputdir/converted will be used
  --filename FILENAME   The input filename
  --nFilesPerJob NFILESPERJOB
                        The number of files to convert per job
  --config CONFIG       The config file to use
```

### How to run
See an example in launchPipeline.sh

