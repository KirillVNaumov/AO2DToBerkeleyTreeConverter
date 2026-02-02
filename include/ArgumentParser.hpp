#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include "logger.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

class ArgumentParser {
public:
  std::string inputFilelist;
  std::string outputFilename = "BerkeleyTree.root";
  std::string configFile = "tree-cuts.yaml";
  bool createHistograms = false;
  bool saveClusters = false;
  bool isMC = false;

  void displayHelp() {
    std::cout << "./converter [args]" << std::endl;
    std::cout << "\t--input-filelist=<file>, -i <file>  : text file with paths to AO2Ds" << std::endl;
    std::cout << "\t--output-filename=<file>, -o <file> : output ROOT file in the BerkeleyTree format (default: \"BerkeleyTree.root\")" << std::endl;
    std::cout << "\t--config-file=<file>, -c <file>     : YAML files with cuts to be done to the converted data (default: \"tree-cuts.yaml\")" << std::endl;
    std::cout << "\t--create-histograms                 : Create histograms from the converted data" << std::endl;
    std::cout << "\t--save-clusters                     : Save clusters" << std::endl;
    std::cout << "\t--is-mc                             : The data is produced from Monte Carlo Simulation" << std::endl;
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
      if (!arg.compare("-i") || !arg.compare("--input-filelist")) {
        if (++iter == canonical_args.end())
          reportError("No input file list after -i/--input-filelist directive");
        inputFilelist = *iter;
      } else if (!arg.compare("-o") || !arg.compare("--output-filename")) {
        if (++iter == canonical_args.end())
          reportError("No output file name after -o/--output-filename directive");
        outputFilename = *iter;
      } else if (!arg.compare("-c") || !arg.compare("--config-file")) {
        if (++iter == canonical_args.end())
          reportError("No config file after -c/--config-file directive");
        configFile = *iter;
      } else if (!arg.compare("--create-histograms")) {
        createHistograms = true;
      } else if (!arg.compare("--save-clusters")) {
        saveClusters = true;
      } else if (!arg.compare("-is-mc")) {
        isMC = true;
      } else if (iter->compare(0, 2, "-v") == 0) {
        ; // verbosity already parsed but avoid error
      } else if (!arg.compare("-h") || !arg.compare("--help")) {
        displayHelp();
        exit(1);
      } else {
        reportError("Unknown argument: " + arg);
      }
    }

    logInfo("Verbosity level: ", getSeverity());
    if (inputFilelist.empty()) {
      reportError("Input file list is not provided");
    }
  }
};

#endif
