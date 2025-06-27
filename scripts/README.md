# Scripts

This section describes the general usage of the downloader (`download_hyperloop.py`) and the conversion scheduler `schedule_conversion.sh`.

## Table of contents

- [Table of contents](#table-of-contents)
- [The launcher](#the-launcher)
- [The HyperDownloader](#the-hyperdownloader)
  - [Dependencies](#dependencies)
  - [Configuration](#configuration)
  - [Obtaining AO2Ds](#obtaining-ao2ds)
  - [Output](#output)
  - [Failed downloads](#failed-downloads)
- [The conversion scheduler](#the-conversion-scheduler)
  - [Dependencies](#dependencies-1)
  - [Configuration](#configuration-1)
  - [Output](#output-1)

## The launcher

The launcher (`scripts/launch_pipeline_nersc.sh`) steers the downloader and conversion scheduler. For usage information on the launcher, see the [NERSC-specific instructions](https://github.com/alicernc/info/blob/main/conversion_at_perlmutter.md).  The full pipeline is steerable from the launcher, so refer to that as much as possible.

> [!CAUTION]
> The `scripts/launch_pipeline.sh` script is out of date. We are currently keeping it for reference only.

## The HyperDownloader

The downloader validates your AliEn token, identifies all matching files in a given set of Hyperloop directories, and downloads them to a specified location on disk.

### Dependencies

The downloader utilizes only the standard library with the exception of the `rich` module. In addition, it must be run *inside* an `alienv` environment, where it has access to the AliEn tools, such as the token tools `alien-token-info` and `alien-token-init` as well as Grid tools like `alien_cp` and `alien_find`.

### Configuration

The downloader can be configured from the command line with the following options:

- `-i, --input <file>`: Single-line text file containing a comma-delimited list of Hyperloop directories to search. See the [next section](#obtaining-ao2ds) for more information.
- `-o, --output <dir>`: Directory to save the downloaded files. If the directory already exists, it will be emptied before downloading.
- `-f, --filename <file>`: The name of the file to search, by default AO2D.root
- `-n, --nthreads <num>`: The number of threads to use when downloading, by default 10
- `-t, --ntries <num>`: The number of retries when downloading a file, by default 5

> [!WARNING]
> Because the output directory is automatically cleared before downloading, it is very easy to accidentally wipe your previously downloaded data, or worse, delete someone else's data! Using some testing/staging area to test downloads, rather than the final directory you want to store your data in, is recommended. There is a dedicated directory for this purpose on NERSC.

### Obtaining AO2Ds

The conversion operates not on raw AO2Ds but rather on JE derived datasets. These derived datasets are prefixed as "JE_*".

1. You can find a list of all the currently available JE derived datasets by going to the [Hyperloop datasets](https://alimonitor.cern.ch/hyperloop/datasets) on AliMonitor and entering `JE_` in the Name searchbox.
2. Click on the train link in the "Production Name" column corresponding to the dataset you want to convert, then go to the "Submitted jobs" tab near the top.
3. On the far right, above the table, should be a "Copy all output directories" button. Click this and a comma-delimited list of the derived dataset's directories will be put on your clipboard.
4. Paste this into a text file somewhere on the system, and direct the downloader to this path with the `--input` option. You can see some examples in the `hylists/` directory.

### Output

In addition to the downloaded AO2Ds, the downloader will also save

- a log of the download;
- a filelist of all downloaded AO2Ds with their paths on the local system;
- and a README, which contains metadata on the download, the converter, and the downloaded Hyperloop directories.

### Failed downloads

The downloader is configured to attempt a file download some number of times (specified by `--ntries <num>`) before it fails. After the downloader attempts a download of all files, it will retry downloading the files that failed. If files still fail to be downloaded, the downloader will give up. From experience, downloads are slower and fail more often in the evening (most likely due to higher traffic on the Grid during morning European hours) so starting a download in the morning or early afternoon is recommended.

## The conversion scheduler

Once the dataset has been downloaded from the Grid, you can now convert the dataset. We highly recommend using a batch job to do this. The batch job can be scheduled via the conversion scheduler `scripts/schedule_conversion_nersc.sh`. Conversion on the NERSC systems is highly recommended, since all of the dependencies are immediately available to users.

### Dependencies

The scheduler relies only on Bash and a Slurm configuration accessible via `sbatch`, which it will check for when it is run.

### Configuration

The downloader can be configured from the command line with the following options:

- `-p, --path`: Path to the `converter` binary, typically located in `bin/`.
- `-i, --input`: Path to the directory containing the AO2Ds to be converted.
- `-o, --output`: Path to the output directory where converted trees will be placed. If the directory does not exist, it will be made automatically. If the directory already exists, it will be cleared before conversion.
- `--aod-name`: Name of the AO2D files, by default `AO2D.root`.
- `--tree-name`: Name of the converted trees, by default `BerkeleyTrees.root`.
- `-n, --nfiles`: Number of AO2D files to convert into one tree, by default 10.
- `-c, --config`: Configuration YAML file specifying the cuts on various properties when converting, by default the `tree-cuts.yaml` file found in the repository root directory.
- `-e, --email`: Email to be notified at when the conversion job starts and completes. Leave without argument or absent to disable email notification.
- `-t, --test`: Start a test run. One set of files will be converted locally.

> [!WARNING]
> Because the output directory is automatically cleared before converting, it is very easy to accidentally wipe your previous work, or worse, delete someone else's data! Using some testing/staging area to test conversions, rather than the final directory you want to store your data in, is recommended.  There is a dedicated directory for this purpose on NERSC.

### Output

The scheduler will first save a list of AO2Ds found in the input directory. It will then construct a conversion batch script to convert these AO2Ds into BerkeleyTrees. If run in test mode, the scheduler will run this script directly to convert a set of AO2Ds into a single BerkeleyTree, as well as show the standard output to the console. If run in production mode, the scheduler will submit this batch script via `sbatch`. It will also submit a dependency job to save a filelist of the produced trees once they are all converted.
