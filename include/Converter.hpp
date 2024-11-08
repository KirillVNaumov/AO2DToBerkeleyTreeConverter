#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <TClass.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TKey.h>
#include <TLeaf.h>
#include <TList.h>
#include <TTree.h>

#include <yaml-cpp/yaml.h>

#define HISTOGRAMS_DO(defH1, defH2)                                 \
  /* TH1F */                                                        \
  defH1(hTrackPt,       "Track p_{T}",      100, 0,   100)          \
  defH1(hClusterEnergy, "Cluster Energy",   100, 0,   100)          \
  defH1(hClusterEta,    "Cluster #eta",     100, -1,  1)            \
  defH1(hClusterPhi,    "Cluster #phi",     100, 0,   2 * 3.14)     \
  defH1(hNEvents,       "Number of events", 2,   0,   2)            \
  defH1(hEvtVtxX,       "Event vertex x",   100, -10, 10)           \
  defH1(hEvtVtxY,       "Event vertex y",   100, -10, 10)           \
  defH1(hEvtVtxZ,       "Event vertex z",   100, -10, 10)           \
  /* TH2F */                                                        \
  defH2(hClusterM02vsE, "Cluster M02 vs E", 300, 0, 3, 100, 0, 100)

// TODO: MC truth information

class TFile;

class Event;

class Converter {

#define DECLARE_HISTOGRAMS(name, title, nbins, xlop, xup) TH1F *name;

#undef DECLARE_HISTOGRAMS

  TFile *outFile;

  // Histograms for QA purposes
  TList *outputhists;

  TTree *outputTree;

  TH1F *hTrackPt;
  TH1F *hClusterEnergy;
  TH1F *hClusterEta;
  TH1F *hClusterPhi;
  TH1F *hNEvents;
  TH1F *hEvtVtxX;
  TH1F *hEvtVtxY;
  TH1F *hEvtVtxZ;

  TH2F *hClusterM02vsE;

  // cuts for tree production
  YAML::Node treecuts;
  YAML::Node eventCuts;
  YAML::Node trackCuts;

  void createQAHistos();
  void createTree();

  void writeEvents(TTree *tree, std::vector<Event> &events);

  void doEventSelection(std::vector<Event> &events);

  void doAnalysis(std::vector<Event> &events);

  void clearBuffers();

  bool _createHistograms;

  // define global switches
  bool fIsPbPb = false;
  bool fIsMC = false;

public:
  void processFile(TFile *file);

  Converter(TString outputFileName, TString configFile, bool createHistograms)
      : _createHistograms(createHistograms) {
    outFile = new TFile(outputFileName.Data(), "RECREATE");

    treecuts = YAML::LoadFile(configFile.Data());

    if (_createHistograms) {
      createQAHistos();
    }
    createTree();
  }

  ~Converter() {
    outFile->cd();
    // LOOP OVER OUTput hists and write to file
    outputTree->Write();
    if (_createHistograms) {
      outputhists->Write();
    }
  }
};

#endif
