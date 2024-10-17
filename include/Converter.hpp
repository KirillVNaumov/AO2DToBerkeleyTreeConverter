#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#include <TList.h>
#include <TLeaf.h>
#include <TLeaf.h>
#include <TKey.h>
#include <TClass.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>

#include <yaml-cpp/yaml.h>

// Histograms for QA purposes
TList *outputhists;

TH1F *hTrackPt;
TH1F *hClusterEnergy;
TH1F *hClusterEta;
TH1F *hClusterPhi;
TH1F *hNEvents;
TH1F *hEvtVtxX;
TH1F *hEvtVtxY;
TH1F *hEvtVtxZ;

TH2F *hClusterM02vsE;

// define global switches
Bool_t fIsPbPb = kFALSE;
Bool_t fIsMC = kFALSE;

//----
// define output tree
//----
TTree *outputTree;

// TODO: MC truth information

// cuts for tree production
YAML::Node treecuts;
YAML::Node eventCuts;
YAML::Node trackCuts;

class TFile;

class Converter {
#if 0
      hTrackPt = new TH1F("hTrackPt", "Track p_{T}", 100, 0, 100);
  hClusterEnergy = new TH1F("hClusterEnergy", "Cluster Energy", 100, 0, 100);
  hClusterEta = new TH1F("hClusterEta", "Cluster #eta", 100, -1, 1);
  hClusterPhi =
      new TH1F("hClusterPhi", "Cluster #phi", 100, 0, 2 * TMath::Pi());
  hNEvents = new TH1F("hNEvents", "Number of events", 2, 0, 2);
  hEvtVtxX = new TH1F("hEvtVtxX", "Event vertex x", 100, -10, 10);
  hEvtVtxY = new TH1F("hEvtVtxY", "Event vertex y", 100, -10, 10);
  hEvtVtxZ = new TH1F("hEvtVtxZ", "Event vertex z", 100, -10, 10);

  hClusterM02vsE =
      new TH2F("hClusterM02vsE", "Cluster M02 vs E", 300, 0, 3, 100, 0, 100);
#endif


public:
    void processFile(TFile *file);

};

#endif
