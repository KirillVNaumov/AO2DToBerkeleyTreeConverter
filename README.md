# AO2DToBerkeleyTreeConverter

This project provides an interface for downloading LHC files in the Run3 format (i.e. AO2D format) and converting them to BerkeleyTrees for use in the analysis infrastructure at LBNL.

To build the converter binary you need to run `make` in the root of the repository. For more debug output during the conversion, run `make BUILD=debug`. To run the converter standalone:
```
./converter [args]
	--inputFileList=<file>, -i <file>  : input ROOT file in the AO2D format
	--outputFileName=<file>, -o <file> : output ROOT file in the BerkeleyTree format (default: "AOD.root")
	--configFile=<file>, -c <file>     : YAML files with cuts to be done to the converted data (default: "treeCuts.yaml")
	--createHistograms                 : Create histograms from the converted data
	--isPbPb                           : The data is from PbPb runs
	--isMC                             : The data is produced from Monte Carlo Simulation
```

