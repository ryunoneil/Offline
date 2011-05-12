//
// An EDAnalyzer module that reads back the hits created by G4 and makes histograms.
//
// $Id: NeutronCRV_plugin.cc,v 1.1 2011/05/12 16:16:47 rhbob Exp $
// $Author: rhbob $
// $Date: 2011/05/12 16:16:47 $
//
// Original author Rob Kutschke
//

// C++ includes
#include <iostream>
#include <string>
#include <cmath>
#include <deque>
#include <map>
#include <list>
#include <memory>

// Framework includes.
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Services/interface/TFileService.h"
#include "FWCore/Framework/interface/TFileDirectory.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Provenance/interface/Provenance.h"

// Root includes.
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TNtuple.h"

// Mu2e includes.
#include "ToyDP/inc/StatusG4.hh"
#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/getTrackerOrThrow.hh"
#include "TrackerGeom/inc/Tracker.hh"
#include "TrackerGeom/inc/Straw.hh"
#include "ITrackerGeom/inc/Cell.hh"
#include "ToyDP/inc/StrawHitCollection.hh"
#include "ToyDP/inc/StrawHitMCTruthCollection.hh"
#include "ToyDP/inc/DPIndexVectorCollection.hh"
#include "ToyDP/inc/StepPointMCCollection.hh"
#include "ToyDP/inc/ToyGenParticleCollection.hh"
#include "ToyDP/inc/SimParticleCollection.hh"
#include "ToyDP/inc/PhysicalVolumeInfoCollection.hh"
#include "ToyDP/inc/CaloHitCollection.hh"
#include "ToyDP/inc/CaloHitMCTruthCollection.hh"
#include "ToyDP/inc/CaloCrystalHitCollection.hh"
#include "ToyDP/inc/CaloCrystalOnlyHitCollection.hh"
#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "Mu2eUtilities/inc/MCCaloUtilities.hh"
#include "ITrackerGeom/inc/ITracker.hh"
#include "TTrackerGeom/inc/TTracker.hh"

#include "CosmicRayShieldGeom/inc/CosmicRayShield.hh"
#include "CosmicRayShieldGeom/inc/CRSScintillatorBar.hh"
#include "CosmicRayShieldGeom/inc/CRSScintillatorBarDetail.hh"
#include "CosmicRayShieldGeom/inc/CRSScintillatorBarIndex.hh"

#include "G4Helper/inc/G4Helper.hh"


// Root includes.
#include "TDirectory.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TNtuple.h"
#include "TGraph.h"

// Other includes.
#include "CLHEP/Units/SystemOfUnits.h"

using namespace std;

using CLHEP::Hep3Vector;
using CLHEP::keV;

namespace mu2e {

  class NeutronCRV : public edm::EDAnalyzer{
  public:

    typedef SimParticleCollection::key_type key_type;

    NeutronCRV(edm::ParameterSet const& pset) : 

      // Run time parameters
      _diagLevel(pset.getUntrackedParameter<int>("diagLevel",0)),
      _g4ModuleLabel(pset.getParameter<string>("g4ModuleLabel")),
      _trackerStepPoints(pset.getUntrackedParameter<string>("trackerStepPoints","tracker")),
      _caloCrystalHitsMaker(pset.getUntrackedParameter<string>("caloCrystalHitsMaker","CaloCrystalHitsMaker")),
      _targetStepPoints(pset.getUntrackedParameter<string>("targetStepPoints","stoppingtarget")),
      _crvStepPoints(pset.getUntrackedParameter<string>("CRVStepPoints","CRV")),
      _minimumEnergy(pset.getParameter<double>("minimumEnergy")),
      _maxFullPrint(pset.getUntrackedParameter<int>("maxFullPrint",500)),
      _xyHitsMax(pset.getUntrackedParameter<int>("xyHitsMax",10000)),

      /*
      // Histograms
      _nAnalyzed(0),
      _hRadius(0),
      _hTime(0),
      _hMultiplicity(0),
      _hDriftDist(0),
      _hDriftDistW(0),
      _hxHit(0),
      _hyHit(0),
      _hzHit(0),
      _hHitNeighbours(0),
      _hCheckPointRadius(0),
      _hCheckPointRadiusW(0),
      _hCheckPointWireZ(0),
      _hMomentumG4(0),
      _hStepLength(0),
      _hEdep(0),
      _hEdepMC(0),
      _hNcrystal(0),
      _hNcrstep(0),
      _hNrostep(0),
      _hEdepROMC(0),
      _hRCEdep(0),
      _hRCTime(0),
      _hRCNCrystals(0),
      _hRCEdepMC(0),
      _hRCTimeMC(0),
      _hRCNCrystalsMC(0),
      _hTargetEdep(0),
      _hTargetPathLength(0),
      _hTargetNfoils(0),
      _hTargetNfoils2D(0),
      _ntup(0),
      _xyHits(0),
      _xyHitCount(0),
      */
      // CRV
      _hCRVMultiplicity(0),
      _ntupCRV(0),
      //
      // Remaining member data
      _nBadG4Status(0){
    }


    virtual void beginJob(edm::EventSetup const&);
    virtual void endJob  ();

    void analyze(edm::Event const& e, edm::EventSetup const&);


  private:

    // Start: run time parameters

    // Diagnostics printout level
    int _diagLevel;

    // Module label of the g4 module that made the hits.
    std::string _g4ModuleLabel;

    // Name of the tracker StepPoint collection
    std::string _trackerStepPoints;

    // Module which made the CaloCrystalHits
    std::string _caloCrystalHitsMaker;
 
    // Name of the stopping target StepPoint collection
    std::string _targetStepPoints;

    // Name of the CRSScintillatorBar(CRV) StepPoint collection
    std::string _crvStepPoints;

    // Cut on the minimum energy.
    double _minimumEnergy;

    // Limit on number of events for which there will be full printout.
    int _maxFullPrint;

    // Limit the size of the TGraph.
    int _xyHitsMax;

    // End: run time parameters

    // Number of events analyzed.
    int _nAnalyzed;

    //ROOT pointers
    TNtuple* _ntup;
    TH1F*    _hCRVMultiplicity;
    TNtuple* _ntupCRV;

    int _nBadG4Status;

    // do the work
    void doCRV(const edm::Event& event);


  };
  
  void NeutronCRV::beginJob(edm::EventSetup const& ){

    // Get access to the TFile service.
    edm::Service<edm::TFileService> tfs;
    
    // CRV

    _hCRVMultiplicity = tfs->make<TH1F>( "hCRVMultiplicity", "CRV StepPointMC per Bar", 5000,  0.,  5000. );

    // Create CRV ntuple.
    _ntupCRV = tfs->make<TNtuple>( "ntupCRV", "CRV Hit ntuple", 
                                   "evt:trk:bid:hx:hy:hz:bx:by:bz:dx:dy:dz:lx:ly:lz:time:shld:mod:lay:pdgId:genId:edep:p:step");

  }

  void NeutronCRV::analyze(const edm::Event& event, edm::EventSetup const&) {
    
    // Maintain a counter for number of events seen.
    ++_nAnalyzed;

    // Inquire about the completion status of G4.
    edm::Handle<StatusG4> g4StatusHandle;
    event.getByLabel( _g4ModuleLabel, g4StatusHandle);
    StatusG4 const& g4Status = *g4StatusHandle;
    if ( _nAnalyzed < _maxFullPrint ){
      cerr << g4Status << endl;
    }

    // Abort if G4 did not complete correctly.  
    // Use your own judgement about whether to abort or to continue.
    if ( g4Status.status() != 0 ) {
      ++_nBadG4Status;
      edm::LogError("G4") 
        << "Aborting NeutronCRV::analyze due to G4 status\n"  
        << g4Status;
      return;
    }
    //
    // get the geometry
    edm::Service<GeometryService> geom;

    if( geom->hasElement<CosmicRayShield>() ) {
      doCRV(event);
    }
  }



  void NeutronCRV::endJob(){
    cout << "NeutronCRV::endJob Number of events skipped "
         << "due to G4 completion status: "
         << _nBadG4Status
         << endl;
  }

  void NeutronCRV::doCRV(const edm::Event& event){

    // Get a reference to CosmicRayShield (it contains crv)


    GeomHandle<CosmicRayShield> cosmicRayShieldGeomHandle;

    std::vector<CRSScintillatorBar> const & allBars = 
      cosmicRayShieldGeomHandle->getAllCRSScintillatorBars();

    CRSScintillatorBarDetail const & barDetail = 
      cosmicRayShieldGeomHandle->getCRSScintillatorBarDetail();

    CLHEP::Hep3Vector barLengths = CLHEP::Hep3Vector(barDetail.getHalfLengths()[0],
                                                     barDetail.getHalfLengths()[1],
                                                     barDetail.getHalfLengths()[2]);

    edm::Handle<StatusG4> g4StatusHandle;
    event.getByLabel( _g4ModuleLabel, g4StatusHandle);
    StatusG4 const& g4Status = *g4StatusHandle;

    // Ask the event to give us a "handle" to the requested hits.
    edm::Handle<StepPointMCCollection> hits;
    event.getByLabel(_g4ModuleLabel,_crvStepPoints,hits);

    // Get handles to the generated and simulated particles.
    edm::Handle<ToyGenParticleCollection> genParticles;
    event.getByType(genParticles);

    edm::Handle<SimParticleCollection> simParticles;
    event.getByType(simParticles);

    // Handle to information about G4 physical volumes.
    edm::Handle<PhysicalVolumeInfoCollection> volumes;
    event.getRun().getByType(volumes);

    // Some files might not have the SimParticle and volume information.
    bool haveSimPart = ( simParticles.isValid() && volumes.isValid() );

    // Other files might have empty collections.
    if ( haveSimPart ){
      haveSimPart = !(simParticles->empty() || volumes->empty());
    }

    // A silly example just to show that we have a messsage logger.
    if ( hits->size() > 300 ){
      edm::LogWarning("HitInfo")
        << "Number of CRV hits "
        << hits->size() 
        << " may be too large.";
    }

    // A silly example just to show how to throw.
    if ( hits->size() > 1000000 ){
      throw cms::Exception("RANGE")
        << "Way too many CRV hits in this event.  Something is really wrong."
        << hits->size();
    }

    if ( _nAnalyzed < _maxFullPrint ){
      cerr << g4Status << endl;
    }

    // ntuple buffer.
    float nt[_ntupCRV->GetNvar()];

    // Loop over all hits.
    for ( size_t i=0; i<hits->size(); ++i ){
      
      // Alias, used for readability.
      const StepPointMC& hit = (*hits)[i];

      // Get the hit information.
      const CLHEP::Hep3Vector& pos = hit.position();
      const CLHEP::Hep3Vector& mom = hit.momentum();
      
      // Get the CRSScintillatorBar information: 
      const CRSScintillatorBar&  bar = allBars.at( hit.volumeId() );
      CLHEP::Hep3Vector const &  mid = bar.getGlobalOffset();
      
      CLHEP::HepRotationX RX(bar.getGlobalRotationAngles()[0]);
      CLHEP::HepRotationY RY(bar.getGlobalRotationAngles()[1]);
      CLHEP::HepRotationZ RZ(bar.getGlobalRotationAngles()[2]);

      CLHEP::HepRotation barInvRotation((RX*RY*RZ).inverse());
      CLHEP::Hep3Vector hitLocal  = barInvRotation*(pos-mid);
      CLHEP::Hep3Vector hitLocalN = CLHEP::Hep3Vector(hitLocal.x()/barLengths.x(),
                                                      hitLocal.y()/barLengths.y(),
                                                      hitLocal.z()/barLengths.z());

      // The simulated particle that made this hit.
      SimParticleCollection::key_type trackId(hit.trackId());

      // Default values for these, in case information is not available.
      int pdgId(0);
      GenId genId;

      if ( haveSimPart ){

        SimParticle const& sim = simParticles->at(trackId);

        // PDG Particle Id of the sim particle that made this hit.
        pdgId = sim.pdgId();

        // If this is a generated particle, which generator did it come from?
        // This default constructs to "unknown".
        if ( sim.fromGenerator() ){
          ToyGenParticle const& gen = genParticles->at(sim.generatorIndex());
          genId = gen.generatorId();
        }
      }
      //
      // as of now I only want to look at neutrons in the counters
      //     cout << "inside Analyze with pdgId = " << pdgId << endl;

      if (pdgId == PDGCode::n0) {
        // Fill the ntuple.
        nt[ 0] = event.id().event();
        nt[ 1] = hit.trackId().asInt();
        nt[ 2] = hit.volumeId();
        nt[ 3] = pos.x();
        nt[ 4] = pos.y();
        nt[ 5] = pos.z();
        nt[ 6] = mid.x();
        nt[ 7] = mid.y();
        nt[ 8] = mid.z();
        nt[ 9] = fabs((pos-mid).x());
        nt[10] = fabs((pos-mid).y());
        nt[11] = fabs((pos-mid).z());
        nt[12] = hitLocalN.x();
        nt[13] = hitLocalN.y();
        nt[14] = hitLocalN.z();
        nt[15] = hit.time();
        nt[16] = bar.Id().getShieldNumber();
        nt[17] = bar.Id().getModuleNumber();
        nt[18] = bar.Id().getLayerNumber();
        nt[19] = pdgId;
        nt[20] = genId.Id();
        nt[21] = hit.eDep()/keV;
        nt[22] = mom.mag();
        nt[23] = hit.stepLength();

        _ntupCRV->Fill(nt);

        cout << "Readback"
             << " hit: "
             << event.id().event() << " "
             << i                  <<  " "
             << hit.trackId()      << "   "
             << hit.volumeId()     << " "
             << bar.Id()           << " | "
             << pos                << " "
             << mid                << " "
             << (mid-pos)          << " | "
             << hitLocalN          << " | "
             << mom                << " "
             << hit.totalEDep()/keV << " "
             << hit.stepLength()
             << endl;
   
        _hCRVMultiplicity->Fill(hit.volumeId());
      }
      // Print out limited to the first few events.
      if ( _nAnalyzed < _maxFullPrint ){

        cerr << "Readback"
             << " hit: "
             << event.id().event() << " "
             << i                  <<  " "
             << hit.trackId()      << "   "
             << hit.volumeId()     << " "
             << bar.Id()           << " | "
             << pos                << " "
             << mid                << " "
             << (mid-pos)          << " | "
             << hitLocalN          << " | "
             << mom                << " "
             << hit.totalEDep()/keV << " "
             << hit.stepLength()
             << endl;
      }
      
    } // end loop over hits.

  } // end doCRV

}  // end namespace mu2e
                                      

                                       using mu2e::NeutronCRV;
DEFINE_FWK_MODULE(NeutronCRV);
