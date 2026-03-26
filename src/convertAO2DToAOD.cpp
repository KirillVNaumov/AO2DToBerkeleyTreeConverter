#include <TFile.h>
#include <TString.h>

#include "ArgumentParser.hpp"
#include "Converter.hpp"
#include "logger.hpp"

void convertAO2DtoAOD(TString inputFilelist = "",
                      TString outputFilename = "output/test.root",
                      TString configFile = "treeCuts.yaml",
                      bool createHistograms = false,
                      bool saveClusters = false
                    ) {

  // loop over all files in txt file filelist
  std::vector<TString> filelist;
  std::ifstream file(inputFilelist.Data());
  std::string str;
  while (std::getline(file, str)) {
    filelist.push_back(str);
  }

  Converter c(outputFilename.Data(), configFile.Data(), createHistograms, saveClusters);

  for (size_t i = 0; i < filelist.size(); i++) {
    TString filePath = filelist.at(i);
    std::cout << "-> Processing file " << filePath << std::endl;
    TFile *in = new TFile(filePath.Data());
    if (!in) std::runtime_error("TFile " + filePath + "not found!");
    c.processFile(in);
    in->Close();
  }
}

int main(int argc, char **argv) {

  try {
    ArgumentParser parser;
    parser.parse(argc, argv);
    convertAO2DtoAOD(
        /*inputFilelist = */ parser.inputFilelist,
        /*outputFilename = */ parser.outputFilename,
        /*configFile = */ parser.configFile,
        /*createHistograms = */ parser.createHistograms,
        /*saveClusters = */ parser.saveClusters);
  } catch (int code) {
    std::cout << "Exception caught: " << code << std::endl;
    return code;
  }
  return 0;
}
