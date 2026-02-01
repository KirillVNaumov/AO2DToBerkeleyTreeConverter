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
Int_t fBuffer_RunNumber;
Float_t fBuffer_centrality;
Float_t fBuffer_multiplicity;
Int_t fBuffer_trackOccupancyInTimeRange;
uint16_t fBuffer_eventselection;
uint64_t fBuffer_triggersel;

// track
std::vector<Float_t>  *fBuffer_track_data_eta;
std::vector<Float_t>  *fBuffer_track_data_phi;
std::vector<Float_t>  *fBuffer_track_data_pt;
std::vector<UShort_t> *fBuffer_track_data_label;
std::vector<uint8_t>  *fBuffer_track_data_tracksel;

// cluster
std::vector<Float_t>  *fBuffer_cluster_data_energy;
std::vector<Float_t>  *fBuffer_cluster_data_eta;
std::vector<Float_t>  *fBuffer_cluster_data_phi;
std::vector<Float_t>  *fBuffer_cluster_data_m02;
std::vector<Float_t>  *fBuffer_cluster_data_m20;
std::vector<UShort_t> *fBuffer_cluster_data_ncells;
std::vector<Float_t>  *fBuffer_cluster_data_time;
std::vector<Bool_t>   *fBuffer_cluster_data_isexotic;
std::vector<UShort_t> *fBuffer_cluster_data_distancebadchannel;
std::vector<UShort_t> *fBuffer_cluster_data_nlm;
std::vector<UShort_t> *fBuffer_cluster_data_clusterdef;
std::vector<UShort_t> *fBuffer_cluster_data_matchedTrackN;
std::vector<Float_t>  *fBuffer_cluster_data_matchedTrackEta;
std::vector<Float_t>  *fBuffer_cluster_data_matchedTrackPhi;
std::vector<Float_t>  *fBuffer_cluster_data_matchedTrackP;

template<class T>
void GetLeafValue (TTree *tree, const char* name, T& container) {
  TBranch *branch = tree->GetBranch(name);
  assert(branch);
  TLeaf *leaf = branch->GetLeaf(name);
  assert(leaf);
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
// distancebadchannel, nlm, clusterdef, leadcellenergy, subleadcellenergy,
// leadingcellnumber, subleadingcellnumber, matchedTrackN, matchedTrackPhi, matchedTrackEta
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
  Bool_t isexotic;
  Float_t distancebadchannel;
  Int_t nlm;
  Int_t clusterdef;
  Float_t leadcellenergy;
  Float_t subleadcellenergy;
  Int_t leadingcellnumber;
  Int_t subleadingcellnumber;
  Int_t matchedTrackN = 0;
  std::vector<Double_t> matchedTrackEta;
  std::vector<Double_t> matchedTrackPhi;
  std::vector<Double_t> matchedTrackP;

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
    GetLeafValue(tree, "fIsExotic", isexotic);
    GetLeafValue(tree, "fDistanceToBadChannel", distancebadchannel);
    GetLeafValue(tree, "fNLM", nlm);
    GetLeafValue(tree, "fDefinition", clusterdef);
    GetLeafValue(tree, "fLeadingCellEnergy", leadcellenergy);
    GetLeafValue(tree, "fSubleadingCellEnergy", subleadcellenergy);
    GetLeafValue(tree, "fLeadingCellNumber", leadingcellnumber);
    GetLeafValue(tree, "fSubleadingCellNumber", subleadingcellnumber);
  }

  void getMatchedTracks(const TTreeReaderArray<Int_t> &matchedTrackIdxs,
                        const std::unordered_map<Int_t, std::tuple<Float_t, Float_t, Float_t>>& matchedTrackMap) {
    // if no matched tracks, skip (number of matched set to zero already)
    if (matchedTrackIdxs.IsEmpty()) {
      return;
    }
    matchedTrackN = matchedTrackIdxs.GetSize();

    for (const Int_t& matchedTrackIdx: matchedTrackIdxs) {
      auto it = matchedTrackMap.find(matchedTrackIdx);
      if (it == matchedTrackMap.end()) {
        // shouldn't happen
        std::cerr << "Matched track not found in map" << std::endl;
        assert(false);
      }
      matchedTrackEta.push_back(std::get<0>(it->second));
      matchedTrackPhi.push_back(std::get<1>(it->second));
      matchedTrackP  .push_back(std::get<2>(it->second));
    }
  }
};

// collision structure
// posx, posy, posz, multiplicity, centrality, eventsel
struct Collision {
  Int_t runNumber;
  Float_t posx;
  Float_t posy;
  Float_t posz;
  Float_t multiplicity;
  Float_t centrality;
  Int_t trackOccupancyInTimeRange;
  uint16_t eventsel; // this should be a uint16_t
  uint64_t triggersel;

  void build(TTree *tree) {
    // fill collision
    GetLeafValue(tree, "fPosX", posx);
    GetLeafValue(tree, "fPosY", posy);
    GetLeafValue(tree, "fPosZ", posz);
    GetLeafValue(tree, "fMultiplicity", multiplicity);
    GetLeafValue(tree, "fCentrality", centrality);
    GetLeafValue(tree, "fEventSel", eventsel);
    GetLeafValue(tree, "fTriggerSel", triggersel);
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
  std::unordered_map<Int_t, std::tuple<Float_t, Float_t, Float_t>> matchedTrackMap;

  TTreeReaderArray<Int_t> matchedTrackIdxs(*clustertracks, "fIndexArrayJTracks");

  // loop over tracks and fill map
  if (!collisions) {
    DEBUG("-> Collisions not found")
    return events;
  }

  // loop over all tracks and fill map
  if (tracks) {
    for (int j = 0; j < tracks->GetEntries(); j++) {
      tracks->GetEntry(j);
			int collisionID;
      GetLeafValue(tracks, "fIndexJCollisions", collisionID);
      trackMap[collisionID].push_back(j);
    }
  } else {
    DEBUG("-> Tracks not found: skipping")
  }

  // loop over all clusters and fill map
  if (clusters) {
    for (int j = 0; j < clusters->GetEntries(); j++) {
      clusters->GetEntry(j);
      int collisionID;
      GetLeafValue(clusters, "fIndexJCollisions", collisionID);
      clusterMap[collisionID].push_back(j);
    }
  } else {
    DEBUG("-> Clusters not found: skipping");
  }
  // check that we have exactly one clustertrack entry for each cluster
  assert(clusters->GetEntries() == clustertracks->GetEntries());

  TTreeReaderValue<Int_t> trackIdx(*emctracks, "fIndexJTracks");
  TTreeReaderValue<Float_t> phi(*emctracks, "fPhiEMCAL");
  TTreeReaderValue<Float_t> eta(*emctracks, "fEtaEMCAL");
  while (emctracks->Next()) {
    tracks->GetEntry(*trackIdx);
    Float_t trackPt, trackEta, trackP;
    GetLeafValue(tracks, "fPt", trackPt);
    GetLeafValue(tracks, "fEta", trackEta);
    trackP = trackPt * cosh(trackEta);
    // map each track index to its etaEMCAL, phiEMCAL, and momentum
    matchedTrackMap.try_emplace(*trackIdx, *eta, *phi, trackP);
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
      if (collisionID != idxCol) {
        // shouldn't happen
        assert(false);
      }
      Track tr;
      tr.build(tracks);
      ev.tracks.push_back(tr);
    }

    // DEBUG("Number of clusters: " << clusterMap[idxCol].size())
    // loop through global indices of tracks (idxTrack) for this collision
    for(const int& idxCluster : clusterMap[idxCol]) {
      clusters->GetEntry(idxCluster);
      int collisionID;
      GetLeafValue(clusters, "fIndexJCollisions", collisionID);
      if (collisionID != idxCol) {
        // shouldn't happen
        assert(false);
      }
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
