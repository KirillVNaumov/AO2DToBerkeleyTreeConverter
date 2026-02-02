#ifndef _eventbuilding_h_included
#define _eventbuilding_h_included

#include "debug.hpp"

#include <unordered_map>
#include <cmath>
#include <tuple>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

// collision
Int_t fBuffer_runNumber;
uint16_t fBuffer_eventSel;
uint64_t fBuffer_triggerSel;
Int_t fBuffer_trackOccupancyInTimeRange;
Float_t fBuffer_centrality;
Float_t fBuffer_multiplicity;

// track
std::vector<Float_t>  *fBuffer_track_data_eta;
std::vector<Float_t>  *fBuffer_track_data_phi;
std::vector<Float_t>  *fBuffer_track_data_pt;
std::vector<UShort_t> *fBuffer_track_data_label;
std::vector<uint8_t>  *fBuffer_track_data_sel;

// cluster
std::vector<Float_t>  *fBuffer_cluster_data_energy;
std::vector<Float_t>  *fBuffer_cluster_data_eta;
std::vector<Float_t>  *fBuffer_cluster_data_phi;
std::vector<Float_t>  *fBuffer_cluster_data_m02;
std::vector<Float_t>  *fBuffer_cluster_data_m20;
std::vector<UShort_t> *fBuffer_cluster_data_ncells;
std::vector<Float_t>  *fBuffer_cluster_data_time;
std::vector<Bool_t>   *fBuffer_cluster_data_isExotic;
std::vector<UShort_t> *fBuffer_cluster_data_distanceToBadChannel;
std::vector<UShort_t> *fBuffer_cluster_data_nlm;
std::vector<UShort_t> *fBuffer_cluster_data_definition;
std::vector<UShort_t> *fBuffer_cluster_data_matchedTrackN;
std::vector<Float_t>  *fBuffer_cluster_data_matchedTrackDeltaEta;
std::vector<Float_t>  *fBuffer_cluster_data_matchedTrackDeltaPhi;
std::vector<Float_t>  *fBuffer_cluster_data_matchedTrackP;
std::vector<uint8_t>  *fBuffer_cluster_data_matchedTrackSel;

template<class T>
void GetLeafValue (TTree *tree, const char* name, T& container) {
  TBranch *branch = tree->GetBranch(name);
  if (!branch) throw std::runtime_error("Branch '" + std::string(name) + "' in TTree " + tree->GetName() + " not found");
  TLeaf *leaf = branch->GetLeaf(name);
  if (!leaf) throw std::runtime_error("Leaf '" + std::string(name) + "' in branch " + branch->GetName() + " not found");
  container = leaf->GetValue();
}

// track structure
// pt, eta, phi, trackSel
struct Track {
  Float_t pt;
  Float_t eta;
  Float_t phi;
  uint8_t trackSel; // check types. This should be a uint8_t

  void build(TTree *tree) {
    GetLeafValue(tree, "fPt", pt);
    GetLeafValue(tree, "fEta", eta);
    GetLeafValue(tree, "fPhi", phi);
    GetLeafValue(tree, "fTrackSel", trackSel);
  }
};

// create cluster structure
// energy, coreEnergy, rawEnergy, eta, phi, m02, m20, ncells, time, isexotic,
// distanceToBadChannel, nlm, definition, leadcellenergy, subleadcellenergy,
// leadcellnumber, subleadcellnumber, matchedTrackN, matchedTrackDeltaPhi, matchedTrackDeltaEta, matchedTrackP, matchedTrackSel
struct Cluster {
  Float_t energy;
  Float_t coreEnergy;
  Float_t rawEnergy;
  Float_t eta;
  Float_t phi;
  Float_t m02;
  Float_t m20;
  Int_t ncells;
  Float_t time;
  Bool_t isExotic;
  Float_t distanceToBadChannel;
  Int_t nlm;
  Int_t definition;
  Float_t leadCellEnergy;
  Float_t subleadCellEnergy;
  Int_t leadCellNumber;
  Int_t subleadCellNumber;
  Int_t matchedTrackN = 0;
  std::vector<Double_t> matchedTrackDeltaEta;
  std::vector<Double_t> matchedTrackDeltaPhi;
  std::vector<Double_t> matchedTrackP;
  std::vector<uint8_t>  matchedTrackSel;

  void build(TTree *tree) {
    GetLeafValue(tree, "fEnergy", energy);
    GetLeafValue(tree, "fCoreEnergy", coreEnergy);
    GetLeafValue(tree, "fRawEnergy", rawEnergy);
    GetLeafValue(tree, "fEta", eta);
    GetLeafValue(tree, "fPhi", phi);
    GetLeafValue(tree, "fM02", m02);
    GetLeafValue(tree, "fM20", m20);
    GetLeafValue(tree, "fNCells", ncells);
    GetLeafValue(tree, "fTime", time);
    GetLeafValue(tree, "fIsExotic", isExotic);
    GetLeafValue(tree, "fDistanceToBadChannel", distanceToBadChannel);
    GetLeafValue(tree, "fNLM", nlm);
    GetLeafValue(tree, "fDefinition", definition);
    GetLeafValue(tree, "fLeadingCellEnergy", leadCellEnergy);
    GetLeafValue(tree, "fSubleadingCellEnergy", subleadCellEnergy);
    GetLeafValue(tree, "fLeadingCellNumber", leadCellNumber);
    GetLeafValue(tree, "fSubleadingCellNumber", subleadCellNumber);
  }

  void getMatchedTracks(const TTreeReaderArray<Int_t> &matchedTrackIdxs,
                        const std::unordered_map<Int_t, std::tuple<Float_t, Float_t, Float_t, uint8_t>>& matchedTrackMap) {
    // if no matched tracks, skip (number of matched set to zero already)
    if (matchedTrackIdxs.IsEmpty()) {
      return;
    }
    matchedTrackN = matchedTrackIdxs.GetSize();

    for (const Int_t& matchedTrackIdx: matchedTrackIdxs) {
      auto it = matchedTrackMap.find(matchedTrackIdx);
      // should be impossible
      if (it == matchedTrackMap.end()) throw std::runtime_error("Matched track not found in cluster-track map!");
      matchedTrackDeltaEta.push_back(std::get<0>(it->second) - eta);
      matchedTrackDeltaPhi.push_back(std::get<1>(it->second) - phi);
      matchedTrackP       .push_back(std::get<2>(it->second));
      matchedTrackSel     .push_back(std::get<3>(it->second));
    }
  }
};

// collision structure
// vertex position, multiplicity, centrality, occupancy, event selection, trigger selection
struct Collision {
  Int_t runNumber;
  Float_t posX;
  Float_t posY;
  Float_t posZ;
  Float_t multiplicity;
  Float_t centrality;
  Int_t trackOccupancyInTimeRange;
  uint16_t eventSel; // this should be a uint16_t
  uint64_t triggerSel;

  void build(TTree *tree) {
    // fill collision
    GetLeafValue(tree, "fPosX", posX);
    GetLeafValue(tree, "fPosY", posY);
    GetLeafValue(tree, "fPosZ", posZ);
    GetLeafValue(tree, "fMultFT0C", multiplicity);
    GetLeafValue(tree, "fCentFT0C", centrality);
    GetLeafValue(tree, "fEventSel", eventSel);
    GetLeafValue(tree, "fTriggerSel", triggerSel);
    GetLeafValue(tree, "fTrackOccupancyInTimeRange", trackOccupancyInTimeRange);
  }
};

// event class containing collision and vector of tracks and clusters
class Event {
public:
  Collision col;
  std::vector<Track> tracks;
  std::vector<Cluster> clusters;
};

std::vector<Event> buildEvents(TTree *collisions, TTree *bc, TTree *tracks,
                               TTree *clusters, TTreeReader *clustertracks,
                               TTreeReader *emctracks) {

  std::vector<Event> events;
  // print all branches and type in the trees
  // collisions->Print();

  // print track table
  //   clusters->Print();
  // loop over collisions
  DEBUG("-> Looping over " << collisions->GetEntries() << " collisions")

  // map of collision index -> track indices for collision
  std::unordered_map<int, std::vector<int>> trackMap;
  // map of collision index -> cluster indices for collision
  std::unordered_map<int, std::vector<int>> clusterMap;
  // map of track index of matched tracks -> track's etaEMCAL, phiEMCAL, momentum
  std::unordered_map<Int_t, std::tuple<Float_t, Float_t, Float_t, uint8_t>> matchedTrackMap;

  TTreeReaderArray<Int_t> matchedTrackIdxs(*clustertracks, "fIndexArrayJTracks");

  // loop over all tracks and fill map
  for (int j = 0; j < tracks->GetEntries(); j++) {
    tracks->GetEntry(j);
    int collisionID;
    GetLeafValue(tracks, "fIndexJCollisions", collisionID);
    trackMap[collisionID].push_back(j);
  }

  // loop over all clusters and fill map
  for (int j = 0; j < clusters->GetEntries(); j++) {
    clusters->GetEntry(j);
    int collisionID;
    GetLeafValue(clusters, "fIndexJCollisions", collisionID);
    clusterMap[collisionID].push_back(j);
  }
  // check that we have exactly one clustertrack entry for each cluster
  if (clusters->GetEntries() != clustertracks->GetEntries())
    throw std::runtime_error("Unequal number of clusters and clustertracks!");

  TTreeReaderValue<Int_t> matchedTrackIdx(*emctracks, "fIndexJTracks");
  TTreeReaderValue<Float_t> phi(*emctracks, "fPhiEMCAL");
  TTreeReaderValue<Float_t> eta(*emctracks, "fEtaEMCAL");
  while (emctracks->Next()) {
    tracks->GetEntry(*matchedTrackIdx);
    Float_t matchedTrackPt, matchedTrackEta, matchedTrackP;
    uint8_t matchedTrackSel;
    GetLeafValue(tracks, "fPt", matchedTrackPt);
    GetLeafValue(tracks, "fEta", matchedTrackEta);
    GetLeafValue(tracks, "fTrackSel", matchedTrackSel);
    matchedTrackP = matchedTrackPt * cosh(matchedTrackEta);
    // map each track index to its etaEMCAL, phiEMCAL, and momentum
    matchedTrackMap.try_emplace(*matchedTrackIdx, *eta, *phi, matchedTrackP, matchedTrackSel);
  }

  for (int idxCol = 0; idxCol < collisions->GetEntries(); idxCol++) {
    collisions->GetEntry(idxCol);
    Event ev;
    //TODO: is it guaranteed a single DF has the same run number?
    GetLeafValue(bc, "fRunNumber", ev.col.runNumber);
    // add this collision to event
    ev.col.build(collisions);

    // DEBUG("Number of tracks: " << trackMap[idxCol].size())
    // loop through global indices of tracks (idxTrack) for this collision
    for(const int& idxTrack : trackMap[idxCol]) {
      tracks->GetEntry(idxTrack);
      int collisionID;
      GetLeafValue(tracks, "fIndexJCollisions", collisionID);
      // check collision IDs match, mismatch should be impossible
      if (collisionID != idxCol)
        throw std::runtime_error("Collision IDs don't match in track map!");
      Track tr;
      tr.build(tracks);
      ev.tracks.push_back(tr);
    }

    // DEBUG("Number of clusters: " << clusterMap[idxCol].size())
    // loop through global indices of clusters (idxCluster) for this collision
    for(const int& idxCluster : clusterMap[idxCol]) {
      clusters->GetEntry(idxCluster);
      int collisionID;
      GetLeafValue(clusters, "fIndexJCollisions", collisionID);
      // check collision IDs match, mismatch should be impossible
      if (collisionID != idxCol)
        throw std::runtime_error("Collision IDs don't match in cluster map!");
      Cluster cl;
      cl.build(clusters);
      clustertracks->SetEntry(idxCluster);
      cl.getMatchedTracks(matchedTrackIdxs, matchedTrackMap);
      ev.clusters.push_back(cl);
    }

    events.push_back(ev);
  }
  return events;
}

#endif
