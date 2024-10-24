#include <TFile.h>
#include <TString.h>
#include "Converter.hpp"

#include <fstream>
#include <iostream>

void writeAnalysisTree(TString inputFileList = "",
                       TString outputFileName = "output/test.root",
                       TString configFile = "treeCuts.yaml",
                       Bool_t createHistograms = false,
                       Bool_t isPbPb = kFALSE, Bool_t isMC = kFALSE) {

  // loop over all files in txt file fileList
  std::vector<TString> fileList;
  std::ifstream file(inputFileList.Data());
  std::string str;
  while (std::getline(file, str)) {
    fileList.push_back(str);
  }

  Converter c(outputFileName.Data(), configFile.Data(), createHistograms);

  for (Int_t i = 0; i < fileList.size(); i++) {
    TString filePath = fileList.at(i);
    std::cout << "-> Processing file " << filePath << std::endl;
    TFile *in = new TFile(filePath.Data());
    c.processFile(in);
    in->Close();
  }
}

int main(int args, char **argv) {

  try {
    /// TODO: Add handling of options via command line
    writeAnalysisTree(
      /*inputFileList = */ "./input_files.txt",
      /*outputFileName = */ "output.root",
      /*configFile = */ "treeCuts.yaml",
      /*createHistograms = */ false,
      /*isPbPb = */ false,
      /*isMC = */ false);
  } catch (int code) {
    std::cout << "Exception caught: " << code << std::endl;
  }
  return 1;
}
