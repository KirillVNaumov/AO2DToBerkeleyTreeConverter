
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

This package comes in three components: a downloader and a conversion scheduler.

- The downloader requires a valid Python 3 installation with the `rich` module installed, and the AliEn tools, in particular `alien_find` and `alien_cp`.
- The converter depends only on `ROOT` and the `yaml-cpp` development package and is compiled with `make`. The name of the development package varies from system to system, but is typically called `yaml-cpp-dev` or `yaml-cpp-devel`.
- The scheduler, written in Bash, requires an accessible Slurm configuration. The jobs are scheduled via `sbatch`.

## Usage

Clone the repository with `git clone git@github.com:KirillVNaumov/AO2DToBerkeleyTreeConverter.git`.

For general usage and documentation, please refer to the [README](scripts/README.md) in the `scripts` directory.

## Future improvements

Check our [issues](https://github.com/KirillVNaumov/AO2DToBerkeleyTreeConverter/issues) page!
