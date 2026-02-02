#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include "logger.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

class ArgumentParser {
public:
  std::string inputFileList;
  std::string outputFileName = "AOD.root";
  std::string configFile = "treeCuts.yaml";
  bool createHistograms = false;
  bool isPbPb = false;
  bool isMC = false;
  bool saveClusters = false;

  void displayHelp() {
    std::cout << "./converter [args]" << std::endl;
    std::cout << "\t--inputFileList=<file>, -i <file>  : input ROOT file in the AO2D format" << std::endl;
    std::cout << "\t--outputFileName=<file>, -o <file> : output ROOT file in the BerkeleyTree format (default: \"AOD.root\")" << std::endl;
    std::cout << "\t--configFile=<file>, -c <file>     : YAML files with cuts to be done to the converted data (default: \"treeCuts.yaml\")" << std::endl;
    std::cout << "\t--createHistograms                 : Create histograms from the converted data" << std::endl;
    std::cout << "\t--saveClusters                     : Save clusters" << std::endl;
    std::cout << "\t--isPbPb                           : The data is from PbPb runs" << std::endl;
    std::cout << "\t--isMC                             : The data is produced from Monte Carlo Simulation" << std::endl;
  }

  void reportError(std::string error) {
    displayHelp();
    std::cerr << "Error during parsing: " << error << std::endl;
    exit(1);
  }

  std::vector<std::string> canonicalize(int argc, char **argv) {
    std::vector<std::string> canonical_args;
    for (int i = 0; i < argc; ++i) {
      std::string arg = argv[i];
      int equalSignNumber = std::count(arg.begin(), arg.end(), '=');
      if (equalSignNumber >= 2) {
        reportError("Malconstructed argument: " + arg);
      }
      std::stringstream ss(arg);
      std::string segment;
      while (std::getline(ss, segment, '=')) {
        canonical_args.push_back(segment);
      }
    }
    return canonical_args;
  }

  void parseVerbosity(const std::string& arg) {
    int count = std::count(arg.begin(), arg.end(), 'v');
    decreaseSeverity(count);
  }

  void parse(int argc, char **argv) {
    std::vector<std::string> canonical_args = canonicalize(argc, argv);
    // set verbosity first
    for (auto iter = canonical_args.begin() + 1; iter < canonical_args.end();
         ++iter) {
      if (iter->compare(0, 2, "-v") == 0) parseVerbosity(*iter);
    }

    // parse remaining options
    for (auto iter = canonical_args.begin() + 1; iter < canonical_args.end();
         ++iter) {
      std::string arg = *iter;
      logInfo("Arg: ", arg);
      if (!arg.compare("-i") || !arg.compare("--inputFileList")) {
        if (++iter == canonical_args.end())
          reportError("No input file list after -i/--inputFileList directive");
        inputFileList = *iter;
      } else if (!arg.compare("-o") || !arg.compare("--outputFileName")) {
        if (++iter == canonical_args.end())
          reportError(
              "No output file name after -o/--outputFileName directive");
        outputFileName = *iter;
      } else if (!arg.compare("-c") || !arg.compare("--configFile")) {
        if (++iter == canonical_args.end())
          reportError("No config file after -c/--configFile directive");
        configFile = *iter;
      } else if (!arg.compare("--createHistograms")) {
        createHistograms = true;
      } else if (!arg.compare("--saveClusters")) {
        saveClusters = true;
      } else if (iter->compare(0, 2, "-v") == 0) {
        ; // verbosity already parsed but avoid error
      } else if (!arg.compare("-PbPb")) {
        isPbPb = true;
      } else if (!arg.compare("-MC")) {
        isMC = true;
      } else if (!arg.compare("-h") || !arg.compare("--help")) {
        displayHelp();
        exit(1);
      } else {
        reportError("Unknown argument: " + arg);
      }
    }

    logInfo("Verbosity level: ", getSeverity());
    if (inputFileList.empty()) {
      reportError("Input file list is not provided");
    }
  }
};

#endif
