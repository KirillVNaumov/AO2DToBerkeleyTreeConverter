#ifndef ARGUMENT_PARSER_HPP
#define ARGUMENT_PARSER_HPP

#include "debug.hpp"

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

  void displayHelp() {
    std::cout << "./converter [args]" << std::endl;
    std::cout << "\t--inputFileList=<file>, -i <file>  : " << std::endl;
    std::cout << "\t--outputFileName=<file>, -o <file> : " << std::endl;
    std::cout << "\t--configFile=<file>, -c <file>     : " << std::endl;
    std::cout << "\t--createHistograms                 : " << std::endl;
    std::cout << "\t--isPbPb                           : " << std::endl;
    std::cout << "\t--isMC                             : " << std::endl;
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

  void parse(int argc, char **argv) {
    std::vector<std::string> canonical_args = canonicalize(argc, argv);
    for (auto iter = canonical_args.begin() + 1; iter < canonical_args.end();
         ++iter) {
      std::string arg = *iter;
      DEBUG("Arg: " << arg)
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
      } else if (!arg.compare("-PbPb")) {
        isPbPb = true;
      } else if (!arg.compare("-MC")) {
        isMC = true;
      } else if (!arg.compare("-h") || !arg.compare("--help")) {
        displayHelp();
      } else {
        reportError("Unknown argument: " + arg);
      }
    }
    if (inputFileList.empty()) {
      reportError("Input file list is not provided");
    }
  }
};

#endif
