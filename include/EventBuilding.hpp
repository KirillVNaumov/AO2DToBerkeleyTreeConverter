#ifndef _eventbuilding_h_included
#define _eventbuilding_h_included

#include "logger.hpp"

#include <unordered_map>
#include <cmath>
#include <tuple>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

// collision
Int_t     fBuffer_runNumber;
Float_t   fBuffer_multiplicity;
Float_t   fBuffer_centrality;
Int_t     fBuffer_trackOccupancyInTimeRange;
UShort_t  fBuffer_eventSel;
ULong64_t fBuffer_triggerSel;
UInt_t    fBuffer_rct;

// track
std::vector<Float_t> *fBuffer_track_pt;
std::vector<Float_t> *fBuffer_track_eta;
std::vector<Float_t> *fBuffer_track_phi;
std::vector<UChar_t> *fBuffer_track_sel;

// cluster
std::vector<Float_t> *fBuffer_cluster_energy;
std::vector<Float_t> *fBuffer_cluster_eta;
std::vector<Float_t> *fBuffer_cluster_phi;
std::vector<Float_t> *fBuffer_cluster_m02;
std::vector<Float_t> *fBuffer_cluster_m20;
std::vector<Int_t>   *fBuffer_cluster_ncells;
std::vector<Float_t> *fBuffer_cluster_time;
std::vector<Bool_t>  *fBuffer_cluster_isExotic;
std::vector<Float_t> *fBuffer_cluster_distanceToBadChannel;
std::vector<Int_t>   *fBuffer_cluster_nlm;
std::vector<Int_t>   *fBuffer_cluster_definition;
std::vector<Int_t>   *fBuffer_cluster_matchedTrackN;
std::vector<Float_t> *fBuffer_cluster_matchedTrackDeltaEta;
std::vector<Float_t> *fBuffer_cluster_matchedTrackDeltaPhi;
std::vector<Float_t> *fBuffer_cluster_matchedTrackP;
std::vector<Float_t> *fBuffer_cluster_matchedTrackPt;
std::vector<UChar_t> *fBuffer_cluster_matchedTrackSel;

template<class T>
void GetLeafValue (TTree *tree, const char* name, T& container) {
  TBranch *branch = tree->GetBranch(name);
  if (!branch) throw std::runtime_error("Branch '" + std::string(name) + "' in TTree " + tree->GetName() + " not found");
  TLeaf *leaf = branch->GetLeaf(name);
  if (!leaf) throw std::runtime_error("Leaf '" + std::string(name) + "' in branch " + branch->GetName() + " not found");
  container = leaf->GetValue();
}

// track structure
struct Track {
  Float_t pt;
  Float_t eta;
  Float_t phi;
  UChar_t trackSel;

  void build(TTree *tree) {
    GetLeafValue(tree, "fPt", pt);
    GetLeafValue(tree, "fEta", eta);
    GetLeafValue(tree, "fPhi", phi);
    GetLeafValue(tree, "fTrackSel", trackSel);
  }
};

// cluster structure
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
  std::vector<Float_t> matchedTrackDeltaEta;
  std::vector<Float_t> matchedTrackDeltaPhi;
  std::vector<Float_t> matchedTrackP;
  std::vector<Float_t> matchedTrackPt;
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
                        const std::unordered_map<Int_t, std::tuple<Float_t, Float_t, Float_t, Float_t, uint8_t, Float_t, Float_t, Float_t, Float_t>>& matchedTrackMap) {
    // if no matched tracks, skip (number of matched set to zero already)
    if (matchedTrackIdxs.IsEmpty()) {
      return;
    }
    matchedTrackN = matchedTrackIdxs.GetSize();

    for (const Int_t& matchedTrackIdx: matchedTrackIdxs) {
      auto it = matchedTrackMap.find(matchedTrackIdx);
      // should be impossible
      if (it == matchedTrackMap.end()) throw std::runtime_error("Matched track not found in cluster-track map!");
      //TODO: check the EtaDiff and PhiDiff variables from the actual matched track
      // Float_t etadiff = std::get<5>(it->second);
      // Float_t phidiff = std::get<6>(it->second);
      // Float_t etadiffcalc = std::get<0>(it->second) - eta;
      // Float_t phidiffcalc = std::get<1>(it->second) - phi;
      // Float_t etadiffcalc2 = std::get<7>(it->second) - eta;
      // Float_t phidiffcalc2 = std::get<8>(it->second) - phi;
      // logWarning(std::get<3>(it->second), " ", etadiff, " ", etadiffcalc, " ", etadiff - etadiffcalc);
      // logWarning(std::get<3>(it->second), " ", phidiff, " ", phidiffcalc, " ", phidiff - phidiffcalc);
      // logWarning(std::get<3>(it->second), " ", etadiff, " ", etadiffcalc2, " ", etadiff - etadiffcalc2);
      // logWarning(std::get<3>(it->second), " ", phidiff, " ", phidiffcalc2, " ", phidiff - phidiffcalc2);
      matchedTrackDeltaEta.push_back(std::get<0>(it->second) - eta);
      matchedTrackDeltaPhi.push_back(std::get<1>(it->second) - phi);
      matchedTrackP       .push_back(std::get<2>(it->second));
      matchedTrackPt      .push_back(std::get<3>(it->second));
      matchedTrackSel     .push_back(std::get<4>(it->second));
    }
  }
};

// collision structure
struct Collision {
  Int_t runNumber;
  Float_t posX;
  Float_t posY;
  Float_t posZ;
  Float_t multiplicity;
  Float_t centrality;
  Int_t trackOccupancyInTimeRange;
  UShort_t eventSel;
  ULong64_t triggerSel;
  UInt_t rct;

  void build(TTree *tree) {
    // fill collision
    GetLeafValue(tree, "fPosX", posX);
    GetLeafValue(tree, "fPosY", posY);
    GetLeafValue(tree, "fPosZ", posZ);
    GetLeafValue(tree, "fMultFT0C", multiplicity);
    GetLeafValue(tree, "fCentFT0C", centrality);
    GetLeafValue(tree, "fTrackOccupancyInTimeRange", trackOccupancyInTimeRange);
    GetLeafValue(tree, "fEventSel", eventSel);
    GetLeafValue(tree, "fTriggerSel", triggerSel);
    GetLeafValue(tree, "fRct", rct);
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
                               TTreeReader *emctracks, bool saveClusters) {

  std::vector<Event> events;
  logDebug("-> Looping over ", collisions->GetEntries(), " collisions");

  // map of collision index -> track indices for collision
  std::unordered_map<int, std::vector<int>> trackMap;
  // map of collision index -> cluster indices for collision
  std::unordered_map<int, std::vector<int>> clusterMap;
  // map of track index of matched tracks -> track's etaEMCAL, phiEMCAL, momentum
  std::unordered_map<Int_t, std::tuple<Float_t, Float_t, Float_t, Float_t, uint8_t, Float_t, Float_t, Float_t, Float_t>> matchedTrackMap;
  TTreeReaderArray<Int_t> matchedTrackIdxs(*clustertracks, "fIndexArrayJTracks");

  // loop over all tracks and fill map
  for (int j = 0; j < tracks->GetEntries(); j++) {
    tracks->GetEntry(j);
    int collisionID;
    GetLeafValue(tracks, "fIndexJCollisions", collisionID);
    trackMap[collisionID].push_back(j);
  }

  if (saveClusters) {
    // check that we have exactly one clustertrack entry for each cluster
    if (clusters->GetEntries() != clustertracks->GetEntries())
      throw std::runtime_error("Unequal number of clusters and clustertracks!");

    // loop over all clusters and fill map
    for (int j = 0; j < clusters->GetEntries(); j++) {
      clusters->GetEntry(j);
      int collisionID;
      GetLeafValue(clusters, "fIndexJCollisions", collisionID);
      clusterMap[collisionID].push_back(j);
    }

    // loop over matched tracks and build matched track map
    TTreeReaderValue<Int_t> matchedTrackIdx(*emctracks, "fIndexJTracks");
    TTreeReaderValue<Float_t> etaEMCAL(*emctracks, "fEtaEMCAL");
    TTreeReaderValue<Float_t> phiEMCAL(*emctracks, "fPhiEMCAL");
    TTreeReaderValue<Float_t> etaDiff(*emctracks, "fEtaDiff");
    TTreeReaderValue<Float_t> phiDiff(*emctracks, "fPhiDiff");
    while (emctracks->Next()) {
      tracks->GetEntry(*matchedTrackIdx);
      Float_t matchedTrackPt, matchedTrackEta, matchedTrackP, matchedTrackPhi;
      UChar_t matchedTrackSel;
      GetLeafValue(tracks, "fPt", matchedTrackPt);
      GetLeafValue(tracks, "fEta", matchedTrackEta);
      GetLeafValue(tracks, "fPhi", matchedTrackPhi);
      GetLeafValue(tracks, "fTrackSel", matchedTrackSel);
      matchedTrackP = matchedTrackPt * cosh(matchedTrackEta);
      // map each track index to its etaEMCAL, phiEMCAL, p, pt, sel
      matchedTrackMap.try_emplace(*matchedTrackIdx, *etaEMCAL, *phiEMCAL, matchedTrackP, matchedTrackPt, matchedTrackSel, *etaDiff, *phiDiff, matchedTrackEta, matchedTrackPhi);
    }
  }

  // loop over collisions
  for (int idxCol = 0; idxCol < collisions->GetEntries(); idxCol++) {
    collisions->GetEntry(idxCol);
    Event ev;
    int idxBC;
    GetLeafValue(collisions, "fIndexJBCs", idxBC);
    bc->GetEntry(idxBC);
    GetLeafValue(bc, "fRunNumber", ev.col.runNumber);
    // build collision info
    ev.col.build(collisions);

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

    if (saveClusters) {
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
    }

    events.push_back(ev);
  }
  return events;
}

#endif
