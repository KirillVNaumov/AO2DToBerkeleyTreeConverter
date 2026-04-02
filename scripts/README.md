# Scripts

This section describes the general usage of the downloader (`download_hyperloop.py`) and the conversion scheduler `schedule_conversion.sh`.

## Table of contents

- [Table of contents](#table-of-contents)
- [The configuration file](#the-configuration-file)
- [The downloader](#the-downloader)
  - [Downloader configuration](#downloader-configuration)
  - [Obtaining the Hyperloop directories](#obtaining-the-hyperloop-directories)
  - [Downloader output](#downloader-output)
- [The converter](#the-converter)
  - [Converter configuration](#converter-configuration)
  - [Converter cuts](#converter-cuts)
  - [Converter output](#converter-output)
  - [Test converter](#test-converter)
- [Perlmutter vs. Hiccup](#perlmutter-vs-hiccup)

## The configuration file

The converter is built of two parts: the downloader, which downloads the JE derived dataset from the Grid, and the converter, which converts the downloaded JE derived dataset into the BerkeleyTrees. Both of these are steered by a single YAML configuration file. We recommend storing this in the `config` directory.

The config file has three main sections:

- `dataset`: This defines the name of the directory in which the downloaded JE derived dataset and converted BerkeleyTrees are stored. This directory will go in `/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data`.
- `download`: The downloader settings.
- `convert`: The converter settings.

The path to the config file should be passed to the downloader and the converter with `-c <path/to/config>`. If the config file path is absolute, it will look there. If it is relative, then the downloader will first search for it in the `config` directory. If not found there, it will look for it relative to the current directory.

## The downloader

The downloader validates your AliEn token, identifies all matching files in a given set of Hyperloop directories, and downloads them locally via `alien_cp`. To run the downloader, simply run

```bash
scripts/run_download.sh -c <path/to/config>
```

### Downloader configuration

The downloader is configured from the `download` section of the config file. The downloader requires the following information:

- `train`: The train number that produced the JE derived dataset. Needed to identify the O2Physics tag it was produced with.
- `hyperdirs`: A comma-delimited list of Hyperloop directories containing the JE derived data. See the [instructions below](#obtaining-the-hyperloop-directories).

In addition, you can also specify the following information:

- `filename`: The filename of the derived data files (AO2D.root by default).
- `nthreads`: The number of threads to use when downloading from the Grid (40 by default).
- `ntries`: The number of download attempts per file (5 by default).
- `timeout`: The maximum time in seconds for each download attempt (150 by default). If you keep getting timeout errors, consider lengthening this time limit.

In some cases, the downloader may not successfully download all the files. To retry the remaining files, simply rerun the downloader with the same command as above. Keep doing this until all files are successfully downloaded, extending the timeout if necessary.

### Obtaining the Hyperloop directories

The conversion operates not on raw AO2Ds but rather on JE derived datasets. These derived datasets are prefixed as "JE_*".

1. There are two ways of obtaining the Hyperloop directories for a given JE derived dataset.
   - If the dataset is already defined and named as a Hyperloop dataset, you can find a list of all the currently available JE derived datasets by going to the [Hyperloop datasets](https://alimonitor.cern.ch/hyperloop/datasets) page on Hyperloop and entering `JE_` in the Name searchbox.
   - You can also see the latest productions of JE derived data by train (regardless of whether they have been made into Hyperloop datasets or not) by going to the JE derived data [Hyperloop analysis page](https://alimonitor.cern.ch/hyperloop/view-analysis/50914) and scrolling to the bottom.
2. If on the dataset page, click on the train link in the "Production Name" column corresponding to the dataset you want to convert, then go to the "Submitted jobs" tab near the top. If on the Hyperloop analysis page, click the train, then go to the "Submitted jobs" tab near the top.
3. On the far right, above the table, should be a "Copy all output directories" button. Click this and a comma-delimited list of the derived dataset's directories will be put on your clipboard.
4. Paste the list into your config file.

### Downloader output

The downloader will save, amongst some other unimportant files,

- A list of the downloaded AO2Ds and their paths on the grid in `aod_paths.txt`;
- a log of the download in `download.log`;
  - a log of the latest download retry in `retry.log` and remote paths that were attempted in `retry_paths.txt`;
- a `filelist.txt` of all downloaded AO2Ds with their paths on the local system (which is then read by the converter);
- a `download_done` file, marking the download as fully completed;
- and a `README.md`, which contains metadata on the download, the converter, and the downloaded Hyperloop directories.

## The converter

The converter converts the downloaded JE derived data files into a BerkeleyTree. To run the converter, simply run

```bash
scripts/run_conversion.sh -c <path/to/config>
```

### Converter configuration

The converter is configured from the `convert` section of the config file. The converter can be configured with:

- `test`: Specifies whether the conversion should be run as a test (`True` by default). If `True`, then the converter will attempt one conversion locally. If `False`, it will submit a Slurm job via `sbatch` to convert the whole dataset.
- `tree_name`: Filename of the BerkeleyTrees (BerkeleyTree.root by default).
- `save_clusters`: Specifies whether to save cluster information (False by default).
- `naod`: The number of AO2D files per BerkeleyTree (10 by default).
- `root_spec`: Grid package specification that provides the installation of ROOT (`ROOT/v6-36-04-alice2-2` by default).
- `email`: Email to be notified when the conversion is finished (None by default). If None or an empty string, no notification will be sent.
- `recompile`: Specifies whether to recompile the converter beforehand (False by default)
- `verbosity`: Verbosity level during conversion. 0 is WARNING, 1 is INFO, and 2 or higher is DEBUG (1 by default)

### Converter cuts

Certain cuts on event, track, and cluster properties can also be applied during the conversion. In general, these cuts should be as loose as possible, to allow as many analyses. Remember that any selection that can be done via the converter can also be done on the analysis level, so only apply cuts if they are strictly necessary (or you think there won't be any reason to not need them). The cuts are defined in the `event_cuts`, `track_cuts`, and `cluster_cuts` subsections in the `convert` section of the config file.

In general, if the cut is not specified (i.e. the key is not present in the config file), it is set to a negative value, or it is set to a YAML null value (i.e. `zvtx_cut:` or `zvtx_cut: None` or `zvtx_cut: null`), the corresponding cut is ignored. The only exceptions are the track transverse momentum minimum and the track eta range, which are set to dummy values which should effectively do nothing.

- Event cuts:
  - `zvtx_cut`: events must have a reconstructed vertex z position within +- of the given value.
  - `clus_E_min`: the event must contain at least one cluster with at least this energy. Note that this means that the event must also have one cluster; events with no clusters will fail this cut and be removed. **Be careful setting this to zero**: only negative values are null values! Setting this cut to zero will enforce that the event must have a cluster in it, which may not be what you want. If you want to ignore this cut, set it to a negative value, rather than zero. This cut is also ignored if `save_clusters` is set to `False`.
- Track cuts:
  - `pt_min`: The track must have transverse momentum greater than this value. If unspecified or set to a YAML null value, it is set to -1.
  - `eta_min`, `eta_max`: The track must have pseudorapidity in the specified range. If unspecified or set to a YAML null value, the minimum and maximum are set to -5 and 5.
- Cluster cuts:
  - `definition`: The cluster must have this definition to be saved. The definition ID for different kinds of clusterizers can be found in [EMCALClusters.h](https://github.com/AliceO2Group/O2Physics/blob/master/PWGJE/DataModel/EMCALClusters.h#L35). V1 clusters are definition 0, and the default V3 clusters are definition 10.
  - `E_min`: The cluster must have this minimum energy.

### Converter output

The converter will compile (if necessary) the converter, then construct a conversion batch script to convert these AO2Ds into BerkeleyTrees. If run in test mode, the converter will run this script directly to convert a set of AO2Ds into a single BerkeleyTree, as well as show the standard output to the console. If run in production mode, the converter will submit this batch script via `sbatch`. It will also submit a dependency job to save a filelist of the produced trees once they are all converted. **It is highly recommend testing with `test: True` first before scheduling the full conversion, to make sure all cuts are applied properly and everything looks normal.**

### Test converter

In the case that a dataset hasn't been downloaded from the Grid, but you have an AO2D that you want to convert, use the `test_conversion.sh` script.

```bash
scripts/test_conversion.sh -c <path/to/config> -o <path/to/output/file> -i <path/to/input/AO2D/file>
```

## Perlmutter vs. Hiccup

The downloader and converter is written for running on Perlmutter, and it is highly recommended to **not** try to do this on Hiccup - you will not have the right dependencies. If you need a dataset on hiccup, convert it first on Perlmutter, then ask Tucker for it to be moved to Hiccup. The datasets on Hiccup can be found at `/rstorage/alice/run3/data`.
