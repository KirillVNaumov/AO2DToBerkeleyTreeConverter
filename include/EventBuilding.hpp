#ifndef _eventbuilding_h_included
#define _eventbuilding_h_included

#include "debug.hpp"

#include <unordered_map>

// event level properties
Int_t fBuffer_RunNumber;
Float_t fBuffer_centrality;
Float_t fBuffer_multiplicity;
uint16_t fBuffer_eventselection;
uint64_t fBuffer_triggersel;
// track
std::vector<Float_t> *fBuffer_track_data_eta;
std::vector<Float_t> *fBuffer_track_data_phi;
std::vector<Float_t> *fBuffer_track_data_pt;
std::vector<UShort_t> *fBuffer_track_data_label;
std::vector<uint8_t> *fBuffer_track_data_tracksel;

// cluster
std::vector<Float_t> *fBuffer_cluster_data_energy;
std::vector<Float_t> *fBuffer_cluster_data_eta;
std::vector<Float_t> *fBuffer_cluster_data_phi;
std::vector<Float_t> *fBuffer_cluster_data_m02;
std::vector<Float_t> *fBuffer_cluster_data_m20;
std::vector<UShort_t> *fBuffer_cluster_data_ncells;
std::vector<Float_t> *fBuffer_cluster_data_time;
std::vector<Bool_t> *fBuffer_cluster_data_isexotic;
std::vector<UShort_t> *fBuffer_cluster_data_distancebadchannel;
std::vector<UShort_t> *fBuffer_cluster_data_nlm;
std::vector<UShort_t> *fBuffer_cluster_data_clusterdef;
std::vector<UShort_t> *fBuffer_cluster_data_matchedTrackIndex;

template<class T>
void GetLeafValue (TTree *tree, const char* name, T& container) {
  TBranch *branch = tree->GetBranch(name);
  assert(branch);
  TLeaf *leaf = branch->GetLeaf(name);
  assert(leaf);
  container = leaf->GetValue();
}

// cleare track structure
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
// leadingcellnumber, subleadingcellnumber, matchedTrackIndex
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
  Int_t matchedTrackIndex;

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
    matchedTrackIndex = -1; // TODO: figure out how to get thank info
  }
};

// collision event structure
// posx, posy, posz, multiplicity, centrality, eventsel
struct Collision {
  Int_t runNumber;
  Float_t posx;
  Float_t posy;
  Float_t posz;
  Float_t multiplicity;
  Float_t centrality;
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
  }
};

// create event class which contains collision and vector or tracks and clusters
class Event {
public:
  Collision col;
  std::vector<Track> tracks;
  std::vector<Cluster> clusters;
};

std::vector<Event> buildEvents(TTree *collisions, TTree *bc, TTree *tracks,
                               TTree *clusters, TTree *clustertracks) {

  std::vector<Event> events;
  // print all branches and type in the trees
  // collisions->Print();

  // print track table
  //   clusters->Print();
  // loop over collision
  DEBUG("-> Looping over " << collisions->GetEntries() << " collisions")

  // before loop over collision, create a map of tracks and collisionID
  std::unordered_map<int, std::vector<int>>
      trackMap; // fill once to figure out which entries from the tree we need
                // for a given

  // build map of clusters
  std::unordered_map<int, std::vector<int>> clusterMap;

  // loop over tracks and fill map
  if (!collisions) {
    DEBUG("-> Collisions is not found")
    return events;
  }

  if (tracks) {
    for (int j = 0; j < tracks->GetEntries(); j++) {
      tracks->GetEntry(j);
			int collisionID;
      GetLeafValue(tracks, "fIndexJCollisions", collisionID);
      trackMap[collisionID].push_back(j);
    }
  } else {
    DEBUG("-> Tracks is not found: skipping")
  }

  // loop over clusters and fill map
  if (clusters) {
    for (int j = 0; j < clusters->GetEntries(); j++) {
      clusters->GetEntry(j);
      int collisionID;
      GetLeafValue(clusters, "fIndexJCollisions", collisionID);
      clusterMap[collisionID].push_back(j);
    }
  } else {
    DEBUG("-> Clusters is not found: skipping")
  }

  for (int i = 0; i < collisions->GetEntries(); i++) {
    collisions->GetEntry(i);
    Event ev;
    GetLeafValue(bc, "fRunNumber", ev.col.runNumber);
    ev.col.build(collisions);

    DEBUG("Number of tracks: " << trackMap[i].size())
    // loop over map to find right indeces
    for (size_t j = 0; j < trackMap[i].size(); j++) {
      tracks->GetEntry(trackMap[i].at(j));
      int collisionID;
      GetLeafValue(tracks, "fIndexJCollisions", collisionID);
      if (collisionID != i)
        continue;
      Track tr;
      tr.build(tracks);
      ev.tracks.push_back(tr);
    }

    DEBUG("Number of clusters: " << clusterMap[i].size())
    // loop over clusters and find those that belong to the current collision
    for (size_t c = 0; c < clusterMap[i].size(); c++) {
      clusters->GetEntry(clusterMap[i].at(c));
      int collisionID;
      GetLeafValue(tracks, "fIndexJCollisions", collisionID);
      if (collisionID != i)
        continue;
      Cluster cl;
      cl.build(clusters);
      ev.clusters.push_back(cl);
    }

    // what follows is not the most efficient code, but loop
    events.push_back(ev);
  }
  return events;
}

#endif
