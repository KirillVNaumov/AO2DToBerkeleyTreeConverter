
<h1 align="center">
  <br>
  AO2DToBerkeleyTreeConverter
  <br>
</h1>

<h4 align="center">A converter for JE derived datasets into the BerkeleyTree format.</h4>

<p align="center">
  <a href="#contact">Contact</a> •
  <a href="#dependencies">Dependencies</a> •
  <a href="#installation">Installation</a> •
  <a href="#usage">Usage</a>
</p>

## Contact

Authors: Kirill Naumov, Tucker Hwang, Florian Jonas  
Contact: Tucker via Slack or [tucker_hwang@berkeley.edu](mailto:tucker_hwang@berkeley.edu)

## Dependencies

This package comes in three components: a downloader, a converter, and a scheduler tool.

- The downloader requires a valid Python 3 installation with the `rich` module installed, and the AliEn tools, in particular `alien_find`, `alien_cp`, and `alien_ls`.
- The converter depends only on `ROOT` and the `yaml-cpp` development package and is compiled with `make`. The name of the development package varies from system to system, but is typically called `yaml-cpp-dev` or `yaml-cpp-devel`.
- The scheduler, written in Bash, requires an accessible Slurm configuration. The jobs are scheduled via `sbatch`.

## Installation

To build the converter binary, you can simply run `make` in the root of the repository or run `make -C <path/to/root>`. To compile in debug mode, run with `make BUILD=debug`. The binary is built into an executable called `converter` in the `bin` directory. To run the converter standalone:

```bash
./bin/converter [args]
  --inputFileList=<file>, -i <file>  : Text file list of input AO2D files to be converted
  --outputFileName=<file>, -o <file> : Path to the output BerkeleyTree ROOT file
  --configFile=<file>, -c <file>     : YAML file containing cuts to be implemented on the converted data
  --createHistograms                 : Build QA histograms from the converted data
  --isPbPb                           : The data is from PbPb runs (currently just a placeholder)
  --isMC                             : The data is produced from Monte Carlo Simulation (currently just a placeholder)
```

## Usage

For general usage and documentation, please refer to the [README](scripts/README.md) in the `scripts` directory. For specifics on how to use the converter on the NERSC system, please see the [NERSC-specific instructions](https://github.com/alicernc/info/blob/main/conversion_at_perlmutter.md).
