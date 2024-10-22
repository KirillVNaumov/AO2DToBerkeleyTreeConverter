#ifndef _eventbuilding_h_included
#define _eventbuilding_h_included

#include "debug.hpp"

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

// cleare track structure
// pt, eta, phi, trackSel
struct track {
  Float_t pt;
  Float_t eta;
  Float_t phi;
  uint8_t trackSel; // check types. This should be a uint8_t
};

// create cluster structure
// energy, coreEnergy, rawEnergy, eta, phi, m02, m20, ncells, time, isexotic,
// distancebadchannel, nlm, clusterdef, leadcellenergy, subleadcellenergy,
// leadingcellnumber, subleadingcellnumber, matchedTrackIndex
struct cluster {
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
};

// collision event structure
// posx, posy, posz, multiplicity, centrality, eventsel
struct collision {
  Int_t runNumber;
  Float_t posx;
  Float_t posy;
  Float_t posz;
  Float_t multiplicity;
  Float_t centrality;
  uint16_t eventsel; // this should be a uint16_t
	uint64_t triggersel;
};

// create event class which contains collision and vector or tracks and clusters
class event {
public:
  collision col;
  std::vector<track> tracks;
  std::vector<cluster> clusters;
};

std::vector<event> buildEvents(TTree *collisions, TTree *bc, TTree *tracks,
                               TTree *clusters, TTree *clustertracks) {

  std::vector<event> events;
  // print all branches and type in the trees
  // collisions->Print();

  // print track table
  //   clusters->Print();
  // loop over collision
	DEBUG("-> Looping over " << collisions->GetEntries() << " collisions")

  // before loop over collision, create a map of tracks and collisionID
  std::unordered_map<int, std::vector<int>> trackMap; // fill once to figure out which entries from the tree we need
                // for a given

  // build map of clusters
  std::unordered_map<int, std::vector<int>> clusterMap;

  // loop over tracks and fill map
  if (!collisions) {
    std::cout << "->Collisions is not found" << std::endl;
    return events;
  }

  if (tracks) {
    for (int j = 0; j < tracks->GetEntries(); j++) {
      tracks->GetEntry(j);
      int collisionID = tracks->GetBranch("fIndexJCollisions")
                            ->GetLeaf("fIndexJCollisions")
                            ->GetValue();
      trackMap[collisionID].push_back(j);

    }
  } else {
    std::cout << "->Tracks is not found: skipping" << std::endl;
  }

  // loop over clusters and fill map
  if (clusters) {
    for (int j = 0; j < clusters->GetEntries(); j++) {
      clusters->GetEntry(j);
      int collisionID = clusters->GetBranch("fIndexJCollisions")
                            ->GetLeaf("fIndexJCollisions")
                            ->GetValue();
      clusterMap[collisionID].push_back(j);

    }
  } else {
    std::cout << "->Clusters is not found: skipping" << std::endl;
  }


  for (int i = 0; i < collisions->GetEntries(); i++) {
    collisions->GetEntry(i);
    event ev;

    ev.col.runNumber =
        bc->GetBranch("fRunNumber")->GetLeaf("fRunNumber")->GetValue();
    // fill collision
    ev.col.posx = collisions->GetBranch("fPosX")->GetLeaf("fPosX")->GetValue();
    ev.col.posy = collisions->GetBranch("fPosY")->GetLeaf("fPosY")->GetValue();
    ev.col.posz = collisions->GetBranch("fPosZ")->GetLeaf("fPosZ")->GetValue();
    ev.col.multiplicity = collisions->GetBranch("fMultiplicity")
                              ->GetLeaf("fMultiplicity")
                              ->GetValue();
    ev.col.centrality = collisions->GetBranch("fCentrality")
                            ->GetLeaf("fCentrality")
                            ->GetValue();
    ev.col.eventsel =
        collisions->GetBranch("fEventSel")->GetLeaf("fEventSel")->GetValue();

    ev.col.triggersel = collisions->GetBranch("fTriggerSel")
                            ->GetLeaf("fTriggerSel")
                            ->GetValue();

    DEBUG("Number of tracks: " << trackMap[i].size())

    // loop over map to find right indeces
    for (int j = 0; j < trackMap[i].size(); j++) {
      tracks->GetEntry(trackMap[i].at(j));
      if (tracks->GetBranch("fIndexJCollisions")
              ->GetLeaf("fIndexJCollisions")
              ->GetValue() != i)
        continue;
      track tr;
      tr.pt = tracks->GetBranch("fPt")->GetLeaf("fPt")->GetValue();
      tr.eta = tracks->GetBranch("fEta")->GetLeaf("fEta")->GetValue();
      tr.phi = tracks->GetBranch("fPhi")->GetLeaf("fPhi")->GetValue();
      tr.trackSel =
          tracks->GetBranch("fTrackSel")->GetLeaf("fTrackSel")->GetValue();
      ev.tracks.push_back(tr);
    }

    // loop over clusters and find those that belong to the current collision

    for (int c = 0; c < clusterMap[i].size(); c++) {
      clusters->GetEntry(clusterMap[i].at(c));
      if (clusters->GetBranch("fIndexJCollisions")
              ->GetLeaf("fIndexJCollisions")
              ->GetValue() != i)
        continue;
      cluster cl;
      cl.energy =
          clusters->GetBranch("fEnergy")->GetLeaf("fEnergy")->GetValue();
      cl.coreEnergy = clusters->GetBranch("fCoreEnergy")
                          ->GetLeaf("fCoreEnergy")
                          ->GetValue();
      cl.rawEnergy =
          clusters->GetBranch("fRawEnergy")->GetLeaf("fRawEnergy")->GetValue();
      cl.eta = clusters->GetBranch("fEta")->GetLeaf("fEta")->GetValue();
      cl.phi = clusters->GetBranch("fPhi")->GetLeaf("fPhi")->GetValue();
      cl.m02 = clusters->GetBranch("fM02")->GetLeaf("fM02")->GetValue();
      cl.m20 = clusters->GetBranch("fM20")->GetLeaf("fM20")->GetValue();
      cl.ncells =
          clusters->GetBranch("fNCells")->GetLeaf("fNCells")->GetValue();
      cl.time = clusters->GetBranch("fTime")->GetLeaf("fTime")->GetValue();
      cl.isexotic =
          clusters->GetBranch("fIsExotic")->GetLeaf("fIsExotic")->GetValue();
      cl.distancebadchannel = clusters->GetBranch("fDistanceToBadChannel")
                                  ->GetLeaf("fDistanceToBadChannel")
                                  ->GetValue();
      cl.nlm = clusters->GetBranch("fNLM")->GetLeaf("fNLM")->GetValue();
      cl.clusterdef = clusters->GetBranch("fDefinition")
                          ->GetLeaf("fDefinition")
                          ->GetValue();
      cl.leadcellenergy = clusters->GetBranch("fLeadingCellEnergy")
                              ->GetLeaf("fLeadingCellEnergy")
                              ->GetValue();
      cl.subleadcellenergy = clusters->GetBranch("fSubleadingCellEnergy")
                                 ->GetLeaf("fSubleadingCellEnergy")
                                 ->GetValue();
      cl.leadingcellnumber = clusters->GetBranch("fLeadingCellNumber")
                                 ->GetLeaf("fLeadingCellNumber")
                                 ->GetValue();
      cl.subleadingcellnumber = clusters->GetBranch("fSubleadingCellNumber")
                                    ->GetLeaf("fSubleadingCellNumber")
                                    ->GetValue();
      cl.matchedTrackIndex = -1; // TODO: figure out how to get thank info
      ev.clusters.push_back(cl);
    }

    // what follows is not the most efficient code, but loop

    events.push_back(ev);
  }
  return events;
}

#endif
