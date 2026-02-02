#include "Converter.hpp"

#include "EventBuilding.hpp"

#include "TROOT.h"
#include "TRint.h"


void Converter::createQAHistos() {
  hTrackPt = new TH1F("hTrackPt", "Track p_{T}", 100, 0, 100);
  hClusterEnergy = new TH1F("hClusterEnergy", "Cluster Energy", 100, 0, 100);
  hClusterEta = new TH1F("hClusterEta", "Cluster #eta", 100, -1, 1);
  hClusterPhi = new TH1F("hClusterPhi", "Cluster #phi", 100, 0,
                         2 * /* TMath::Pi()*/ 3.14);
  hNEvents = new TH1F("hNEvents", "Number of events", 2, 0, 2);
  hEvtVtxX = new TH1F("hEvtVtxX", "Event vertex x", 100, -10, 10);
  hEvtVtxY = new TH1F("hEvtVtxY", "Event vertex y", 100, -10, 10);
  hEvtVtxZ = new TH1F("hEvtVtxZ", "Event vertex z", 100, -10, 10);

  hClusterM02vsE =
      new TH2F("hClusterM02vsE", "Cluster M02 vs E", 300, 0, 3, 100, 0, 100);

  outputhists = new TList();
  // add them all to histos list
  outputhists->Add(hTrackPt);
  outputhists->Add(hClusterEnergy);
  outputhists->Add(hClusterEta);
  outputhists->Add(hClusterPhi);
  outputhists->Add(hNEvents);
  outputhists->Add(hEvtVtxX);
  outputhists->Add(hEvtVtxY);
  outputhists->Add(hEvtVtxZ);
  outputhists->Add(hClusterM02vsE);
}

void Converter::createTree() {
  outputTree = new TTree("eventTree", "eventTree");

  outputTree->Branch("run_number", &fBuffer_runNumber);
  outputTree->Branch("event_sel", &fBuffer_eventSel);
  outputTree->Branch("trig_sel", &fBuffer_triggerSel);
  outputTree->Branch("occupancy", &fBuffer_trackOccupancyInTimeRange);
  outputTree->Branch("centrality", &fBuffer_centrality);
  outputTree->Branch("multiplicity", &fBuffer_multiplicity);

  // track
  outputTree->Branch("track_data_eta", &fBuffer_track_data_eta);
  outputTree->Branch("track_data_phi", &fBuffer_track_data_phi);
  outputTree->Branch("track_data_pt", &fBuffer_track_data_pt);
  outputTree->Branch("track_data_label", &fBuffer_track_data_label);
  outputTree->Branch("track_data_sel", &fBuffer_track_data_sel);

  // cluster
  if (_saveClusters) {
    outputTree->Branch("cluster_data_energy", &fBuffer_cluster_data_energy);
    outputTree->Branch("cluster_data_eta", &fBuffer_cluster_data_eta);
    outputTree->Branch("cluster_data_phi", &fBuffer_cluster_data_phi);
    outputTree->Branch("cluster_data_m02", &fBuffer_cluster_data_m02);
    outputTree->Branch("cluster_data_m20", &fBuffer_cluster_data_m20);
    outputTree->Branch("cluster_data_ncells", &fBuffer_cluster_data_ncells);
    outputTree->Branch("cluster_data_time", &fBuffer_cluster_data_time);
    outputTree->Branch("cluster_data_exoticity", &fBuffer_cluster_data_isExotic);
    outputTree->Branch("cluster_data_dbc", &fBuffer_cluster_data_distanceToBadChannel);
    outputTree->Branch("cluster_data_nlm", &fBuffer_cluster_data_nlm);
    outputTree->Branch("cluster_data_def", &fBuffer_cluster_data_definition);
    outputTree->Branch("cluster_data_matched_track_n", &fBuffer_cluster_data_matchedTrackN);
    outputTree->Branch("cluster_data_matched_track_delta_eta", &fBuffer_cluster_data_matchedTrackDeltaEta);
    outputTree->Branch("cluster_data_matched_track_delta_phi", &fBuffer_cluster_data_matchedTrackDeltaPhi);
    outputTree->Branch("cluster_data_matched_track_p", &fBuffer_cluster_data_matchedTrackP);
    outputTree->Branch("cluster_data_matched_track_sel", &fBuffer_cluster_data_matchedTrackSel);
  }
  // outputTree->SetDirectory(0);
}

void Converter::clearBuffers() {
  fBuffer_track_data_eta->clear();
  fBuffer_track_data_phi->clear();
  fBuffer_track_data_pt->clear();
  fBuffer_track_data_label->clear();
  fBuffer_track_data_sel->clear();

  if (_saveClusters) {
    fBuffer_cluster_data_energy->clear();
    fBuffer_cluster_data_eta->clear();
    fBuffer_cluster_data_phi->clear();
    fBuffer_cluster_data_m02->clear();
    fBuffer_cluster_data_m20->clear();
    fBuffer_cluster_data_ncells->clear();
    fBuffer_cluster_data_time->clear();
    fBuffer_cluster_data_isExotic->clear();
    fBuffer_cluster_data_distanceToBadChannel->clear();
    fBuffer_cluster_data_nlm->clear();
    fBuffer_cluster_data_definition->clear();
    fBuffer_cluster_data_matchedTrackN->clear();
    fBuffer_cluster_data_matchedTrackDeltaEta->clear();
    fBuffer_cluster_data_matchedTrackDeltaPhi->clear();
    fBuffer_cluster_data_matchedTrackP->clear();
    fBuffer_cluster_data_matchedTrackSel->clear();
  }
}

// write events to TTree
void Converter::writeEvents(TTree *tree, std::vector<Event> &events) {
  for (auto &ev : events) {
    // clear all buffers
    clearBuffers();

    // CHECK: does this actually do anything? if so, have to be careful of
    // what happens to matched tracks
    if (_saveClusters && treecuts["event_cuts"]["min_clus_E"].as<float>() > 0.) {
      bool acc = false;
      for (auto &cl : ev.clusters) {
        if (cl.energy > treecuts["event_cuts"]["min_clus_E"].as<float>()) {
          acc = true;
          break;
        }
      }
      if (!acc)
        continue;
    }

    // fill event level properties
    // TODO: check types, maybe some way to do this dynamically?
    fBuffer_runNumber = (Int_t)ev.col.runNumber;
    fBuffer_eventSel = (uint16_t)ev.col.eventSel;
    fBuffer_triggerSel = (uint64_t)ev.col.triggerSel;
    fBuffer_trackOccupancyInTimeRange = (Int_t)ev.col.trackOccupancyInTimeRange;
    fBuffer_centrality = (Float_t)ev.col.centrality;
    fBuffer_multiplicity = (Float_t)ev.col.multiplicity;

    trackCuts = treecuts["track_cuts"];
    // fill track properties
    for (auto &tr : ev.tracks) {
      if (tr.pt < trackCuts["min_pt"].as<float>())
        continue;
      if (tr.eta < trackCuts["min_eta"].as<float>())
        continue;
      if (tr.eta > trackCuts["max_eta"].as<float>())
        continue;

      fBuffer_track_data_eta->push_back((Float_t)tr.eta);
      fBuffer_track_data_phi->push_back((Float_t)tr.phi);
      fBuffer_track_data_pt->push_back((Float_t)tr.pt);
      fBuffer_track_data_label->push_back(0.); // TODO placeholder
      fBuffer_track_data_sel->push_back((uint8_t)tr.trackSel);
    }

    // fill cluster properties
    if (_saveClusters) {
      for (auto &cl : ev.clusters) {
        fBuffer_cluster_data_energy->push_back((Float_t)cl.energy);
        fBuffer_cluster_data_eta->push_back((Float_t)cl.eta);
        fBuffer_cluster_data_phi->push_back((Float_t)cl.phi);
        fBuffer_cluster_data_m02->push_back((Float_t)cl.m02);
        fBuffer_cluster_data_m20->push_back((Float_t)cl.m20);
        fBuffer_cluster_data_ncells->push_back((UShort_t)cl.ncells);
        fBuffer_cluster_data_time->push_back((Float_t)cl.time);
        fBuffer_cluster_data_isExotic->push_back((Bool_t)cl.isExotic);
        fBuffer_cluster_data_distanceToBadChannel->push_back(
            (UShort_t)cl.distanceToBadChannel);
        fBuffer_cluster_data_nlm->push_back((UShort_t)cl.nlm);
        fBuffer_cluster_data_definition->push_back((UShort_t)cl.definition);
        fBuffer_cluster_data_matchedTrackN->push_back(
            (UShort_t)cl.matchedTrackN);
        fBuffer_cluster_data_matchedTrackDeltaEta->insert(fBuffer_cluster_data_matchedTrackDeltaEta->end(), cl.matchedTrackDeltaEta.begin(), cl.matchedTrackDeltaEta.end());
        fBuffer_cluster_data_matchedTrackDeltaPhi->insert(fBuffer_cluster_data_matchedTrackDeltaPhi->end(), cl.matchedTrackDeltaPhi.begin(), cl.matchedTrackDeltaPhi.end());
        fBuffer_cluster_data_matchedTrackP->insert(fBuffer_cluster_data_matchedTrackP->end(), cl.matchedTrackP.begin(), cl.matchedTrackP.end());
        fBuffer_cluster_data_matchedTrackSel->insert(fBuffer_cluster_data_matchedTrackSel->end(), cl.matchedTrackSel.begin(), cl.matchedTrackSel.end());
      }
    }

    // fill tree
    tree->Fill();
  }
}

void Converter::doEventSelection(std::vector<Event> &events) {
  // loop over events and remove them from vector if they don't fulfull a
  // certain cut possibility to do event selection if wanted
  eventCuts = treecuts["event_cuts"];

  // loop over event vector and erase if not fulfilling cuts
  std::vector<Event>::iterator it = events.begin();
  while (it != events.end()) {
    if (TMath::Abs(it->col.posZ) > eventCuts["z_vtx_cut"].as<float>()) {
      it = events.erase(it);
    } else if ((it->col.multiplicity < eventCuts["mult_min"].as<float>()) ||
               (it->col.multiplicity > eventCuts["mult_max"].as<float>())) {
      it = events.erase(it);

    } else {
      ++it;
    }
  }
  return;
}

// can be used to do analysis (if needed)
void Converter::doAnalysis(std::vector<Event> &events) {
  for (auto &ev : events) {
    hNEvents->Fill(1);
    hEvtVtxX->Fill(ev.col.posX);
    hEvtVtxY->Fill(ev.col.posY);
    hEvtVtxZ->Fill(ev.col.posZ);

    // plot pt of all tracks
    for (auto &tr : ev.tracks) {
      hTrackPt->Fill(tr.pt);
    }

    // plot energy, eta and phi of all clusters
    for (auto &cl : ev.clusters) {
      hClusterEnergy->Fill(cl.energy);
      hClusterEta->Fill(cl.eta);
      hClusterPhi->Fill(cl.phi);

      hClusterM02vsE->Fill(cl.m02, cl.energy);
    }
  }
}

void Converter::processFile(TFile *file) {
  std::vector<Event> events;
  int totalNumberOfEvents = 0;
  // loop over all directories and print name
  TIter next(file->GetListOfKeys());
  TKey *key;
  int count = 0;
  while ((key = (TKey *)next())) {
    TClass *cl = gROOT->GetClass(key->GetClassName());
    if (!cl->InheritsFrom("TDirectory"))
      continue;
    std::cout << "   Converting dataframe: " << key->GetName() << std::endl;
    TDirectory *dir = (TDirectory *)key->ReadObj();

    TTreeReader *O2jclustertrack = new TTreeReader("O2jclustertrack", dir);
    if (_saveClusters && O2jclustertrack->IsInvalid()) throw std::runtime_error("TTree O2jclustertrack could not be found in file.");
    TTreeReader *O2jemctrack = new TTreeReader("O2jemctrack", dir);
    if (_saveClusters && O2jemctrack->IsInvalid()) throw std::runtime_error("TTree O2jemctrack could not be found in file.");
    TTree *O2jcollision = (TTree *)dir->Get("O2jcollision");
    if (!O2jcollision) throw std::runtime_error("TTree O2jcollision could not be found in file.");
    TTree *O2jtrack = (TTree *)dir->Get("O2jtrack");
    if (!O2jtrack) throw std::runtime_error("TTree O2jtrack could not be found in file.");
    TTree *O2jcluster = (TTree *)dir->Get("O2jcluster");
    if (_saveClusters && !O2jcluster) throw std::runtime_error("TTree O2jcluster could not be found in file.");
    TTree *O2jbc = (TTree *)dir->Get("O2jbc");
    if (!O2jbc) throw std::runtime_error("TTree O2jbc could not be found in file.");

    // build event
    events =
        buildEvents(O2jcollision, O2jbc, O2jtrack, O2jcluster, O2jclustertrack, O2jemctrack, _saveClusters);

    DEBUG("Event size: " << events.size())
    totalNumberOfEvents += events.size();

    // do event selection
    doEventSelection(events);

    if (_createHistograms)
      doAnalysis(events);

    // write events to TTree
    writeEvents(outputTree, events);

    // delete all events
    events.clear();
    count++;
  }

  std::cout << "Total DFs: " << count << std::endl;
  std::cout << "Total events: " << totalNumberOfEvents << std::endl;
}
