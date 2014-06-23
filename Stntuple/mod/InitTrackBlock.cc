//-----------------------------------------------------------------------------
//  Dec 26 2000 P.Murat: initialization of the STNTUPLE track block
//  2014-06-23: remove vane support
//-----------------------------------------------------------------------------
#include <cstdio>
#include "TROOT.h"
#include "TFolder.h"
#include "TLorentzVector.h"
#include "TVector2.h"

#include "Stntuple/obj/TStnTrackBlock.hh"
#include "Stntuple/obj/TStnTrack.hh"

#include "art/Framework/Principal/Selector.h"
#include "art/Framework/Principal/Handle.h"

#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/VirtualDetector.hh"

#include "TTrackerGeom/inc/TTracker.hh"
#include "CalorimeterGeom/inc/Calorimeter.hh"
#include "CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "CalorimeterGeom/inc/VaneCalorimeter.hh"

#include "KalmanTests/inc/KalRepCollection.hh"
#include "TrackCaloMatching/inc/TrkToCaloExtrapolCollection.hh"
#include "TrackCaloMatching/inc/TrackClusterLink.hh"

#include "TrackCaloMatching/inc/TrkToCaloExtrapol.hh"
#include "TrackCaloMatching/inc/TrackClusterMatch.hh"

#include "MCDataProducts/inc/GenParticleCollection.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "MCDataProducts/inc/StepPointMCCollection.hh"
#include "MCDataProducts/inc/StrawHitMCTruthCollection.hh"
#include "MCDataProducts/inc/PtrStepPointMCVectorCollection.hh"
#include "MCDataProducts/inc/VirtualDetectorId.hh"

// Mu2e includes.
#include "KalmanTests/inc/KalRepCollection.hh"
#include "KalmanTests/inc/TrkStrawHit.hh"

#include "RecoDataProducts/inc/StrawHitCollection.hh"
#include "RecoDataProducts/inc/CaloCrystalHitCollection.hh"
#include "RecoDataProducts/inc/CaloHitCollection.hh"
#include "RecoDataProducts/inc/CaloClusterCollection.hh"
#include "RecoDataProducts/inc/PIDProductCollection.hh"

#include "CalPatRec/inc/AlgorithmIDCollection.hh"


namespace {

  struct ZMap_t {
    int    fMap[44][6][2];
    double fZ  [88];
  };


//-----------------------------------------------------------------------------
// Map[ist][isec][il]: index of the layer in Z-ordered sequence
//-----------------------------------------------------------------------------
  void InitTrackerZMap(const mu2e::TTracker* Tracker, ZMap_t* Map) {
    int      ix, loc;
    double   z0, z1;
    for (int ist=0; ist<44; ist++) {
      for (int isec=0; isec<6; isec++) {
	ix  = isec % 2;
	loc = 2*ist+ix;
	Map->fMap[ist][isec][0] = loc;
	Map->fMap[ist][isec][1] = loc;
      }
					// form list of Z-s

      const mu2e::Layer *l0, *l1;

      l0 = &Tracker->getDevice(ist).getSector(0).getLayer(0);
      l1 = &Tracker->getDevice(ist).getSector(0).getLayer(1);
      z0 = l0->straw0MidPoint().z();
      z1 = l1->straw0MidPoint().z();

      Map->fZ[2*ist] = (z0+z1)/2.;

      l0 = &Tracker->getDevice(ist).getSector(1).getLayer(0);
      l1 = &Tracker->getDevice(ist).getSector(1).getLayer(1);
      z0 = l0->straw0MidPoint().z();
      z1 = l1->straw0MidPoint().z();

      Map->fZ[2*ist+1] = (z0+z1)/2.;
    }
  }

//-----------------------------------------------------------------------------
// for a given Z find closest Z-layer
//-----------------------------------------------------------------------------
  void get_station(ZMap_t* Map, double Z, int* Station, int* Offset) {

    double dz, dz_min(1.e10);
    int    ist    = -1;

    for (int i=0; i<88; i++) {
      dz = Map->fZ[i]-Z;

      if (fabs(dz) < dz_min) {
	ist    = i;
	dz_min = fabs(dz);
      }
    }

    *Station = ist / 2;
    *Offset  = ist % 2;
  }

};


//-----------------------------------------------------------------------------
Int_t StntupleInitMu2eTrackBlock  (TStnDataBlock* Block, AbsEvent* AnEvent, Int_t Mode) {

  const char*               oname = {"InitMu2eTrackBlock"};

  static int initialized(0);
  
  int                       ntrk, ev_number, rn_number;
  KalRep                    *krep;  
  double                    h1_fltlen, hn_fltlen, entlen, fitmom_err;
  TStnTrack*                track;
  TStnTrackBlock            *data;   
  const mu2e::StepPointMC*  step;

  static  ZMap_t            zmap;

  const mu2e::TTracker*     tracker;


  mu2e::AlgorithmIDCollection*           list_of_algs(0);
  mu2e::KalRepCollection*                list_of_kreps(0);
  mu2e::PtrStepPointMCVectorCollection*  list_of_mc_straw_hits(0);
  //  mu2e::CaloClusterCollection*           list_of_clusters(0);
  mu2e::StrawHitCollection*              list_of_straw_hits(0);
  mu2e::TrkToCaloExtrapolCollection*     list_of_extrapolated_tracks(0);
  const mu2e::PIDProductCollection*      list_of_pidp(0);

  static char   algs_module_label[100], algs_description[100];
  static char   krep_module_label[100], krep_description[100];
  static char   trex_module_label[100], trex_description[100];
  static char   trcl_module_label[100], trcl_description[100];
  static char   strh_module_label[100], strh_description[100];
  static char   stmc_module_label[100], stmc_description[100];
  static char   calo_module_label[100], calo_description[100];
  static char   pidp_module_label[100], pidp_description[100];
  static char   g4_module_label  [100], g4_description  [100];


  mu2e::GeomHandle<mu2e::TTracker> ttHandle;
  tracker = ttHandle.get();

  if (initialized == 0) {
    InitTrackerZMap(tracker,&zmap);
    initialized = 1;
  }

  ev_number = AnEvent->event();
  rn_number = AnEvent->run();

  if (Block->Initialized(ev_number,rn_number)) return 0;

  data = (TStnTrackBlock*) Block;
  data->Clear();

  data->GetModuleLabel("mu2e::AlgorithmIDCollection",algs_module_label);
  data->GetDescription("mu2e::AlgorithmIDCollection",algs_description );

  data->GetModuleLabel("mu2e::KalRepCollection",krep_module_label);
  data->GetDescription("mu2e::KalRepCollection",krep_description );

  data->GetModuleLabel("mu2e::TrkToCaloExtrapolCollection",trex_module_label);
  data->GetDescription("mu2e::TrkToCaloExtrapolCollection",trex_description );

  data->GetModuleLabel("mu2e::TrackClusterLink",trcl_module_label);
  data->GetDescription("mu2e::TrackClusterLink",trcl_description );

  data->GetModuleLabel("mu2e::StrawHitCollection",strh_module_label);
  data->GetDescription("mu2e::StrawHitCollection",strh_description );
  
  data->GetModuleLabel("mu2e::PtrStepPointMCVectorCollection",stmc_module_label);
  data->GetDescription("mu2e::PtrStepPointMCVectorCollection",stmc_description );

  data->GetModuleLabel("mu2e::CaloClusterCollection",calo_module_label);
  data->GetDescription("mu2e::CaloClusterCollection",calo_description );

  data->GetModuleLabel("mu2e::PIDProductCollection",pidp_module_label);
  data->GetDescription("mu2e::PIDProductCollection",pidp_description );

  data->GetModuleLabel("mu2e::StepPointMCCollection",g4_module_label);
  data->GetDescription("mu2e::StepPointMCCollection",g4_description );

  art::Handle<mu2e::AlgorithmIDCollection> algsHandle;
  strcpy(algs_module_label,"MergePatRec");
  strcpy(algs_description,krep_description);
  if (algs_module_label[0] != 0) {
    if (algs_description[0] == 0) AnEvent->getByLabel(algs_module_label,algsHandle);
    else                          AnEvent->getByLabel(algs_module_label,algs_description, algsHandle);
    if (algsHandle.isValid()) list_of_algs = (mu2e::AlgorithmIDCollection*) algsHandle.product();
  }

  art::Handle<mu2e::KalRepCollection> krepsHandle;
  if (krep_module_label[0] != 0) {
    if (krep_description[0] == 0) AnEvent->getByLabel(krep_module_label,krepsHandle);
    else                          AnEvent->getByLabel(krep_module_label,krep_description, krepsHandle);
    list_of_kreps = (mu2e::KalRepCollection*) krepsHandle.product();
  }

  art::Handle<mu2e::PtrStepPointMCVectorCollection> mcptrHandle;
  if (stmc_module_label[0] != 0) {
    if (stmc_description[0] == 0) AnEvent->getByLabel(stmc_module_label,mcptrHandle);
    else                       AnEvent->getByLabel(stmc_module_label,stmc_description, mcptrHandle);
    if (mcptrHandle.isValid()) {
      list_of_mc_straw_hits = (mu2e::PtrStepPointMCVectorCollection*) mcptrHandle.product();
    }
  }

//   art::Handle<mu2e::CaloClusterCollection> calo_cluster_handle;
//   if (calo_module_label[0] != 0) {
//     if (calo_description[0] == 0) AnEvent->getByLabel(calo_module_label,calo_cluster_handle);
//     else                          AnEvent->getByLabel(calo_module_label,calo_description,calo_cluster_handle);
//     list_of_clusters  = (mu2e::CaloClusterCollection*) &(*calo_cluster_handle);
//  }
 
  art::Handle<mu2e::StrawHitCollection> shHandle;
  if (strh_module_label[0] != 0) {
    if (strh_description[0] == 0) AnEvent->getByLabel(strh_module_label,shHandle);
    else                          AnEvent->getByLabel(strh_module_label,strh_description,shHandle);
    list_of_straw_hits = (mu2e::StrawHitCollection*) &(*shHandle);
    //    int n_straw_hits      = list_of_straw_hits->size();
  }

  art::Handle<mu2e::TrkToCaloExtrapolCollection>  texHandle;
  if (trex_module_label[0] != 0) {
    AnEvent->getByLabel(trex_module_label,trex_description,texHandle);
    list_of_extrapolated_tracks = (mu2e::TrkToCaloExtrapolCollection*) &(*texHandle);
  }

  //  art::Handle<mu2e::TrackClusterLink>  trk_cal_map;
  art::Handle<mu2e::TrackClusterMatchCollection>  tcmH;
  if (trcl_module_label[0] != 0) {
    if (trcl_description[0] == 0) AnEvent->getByLabel(trcl_module_label,tcmH);
    else                          AnEvent->getByLabel(trcl_module_label,trcl_description,tcmH);
  }

  art::Handle<mu2e::PIDProductCollection>  pidpHandle;
  if (pidp_module_label[0] != 0) {
    AnEvent->getByLabel(pidp_module_label,pidp_description,pidpHandle);
    list_of_pidp = pidpHandle.product();
  }

  //  const mu2e::Calorimeter* cal;

  art::ServiceHandle<mu2e::GeometryService> geom;

  int                       cal_type(-1);   // 1:vane, 2:disk
  const mu2e::AlgorithmID*  alg_id;
  int                       mask;

  const mu2e::BaseCalorimeter* bc(NULL);
  
  if (geom->hasElement<mu2e::VaneCalorimeter>() ) {
    mu2e::GeomHandle<mu2e::VaneCalorimeter> h;
    bc = (const mu2e::BaseCalorimeter*) h.get();
    cal_type = 1;
  }
  else if (geom->hasElement<mu2e::DiskCalorimeter>() ) {
    mu2e::GeomHandle<mu2e::DiskCalorimeter> h;
    bc = (const mu2e::BaseCalorimeter*) h.get();
    cal_type = 2;
  }

  ntrk = list_of_kreps->size();

  for (int itrk=0; itrk<ntrk; itrk++) {
    track          = data->NewTrack();
    krep           = (KalRep*) &(*list_of_kreps)[itrk];
//-----------------------------------------------------------------------------
// track-only-based particle ID, initialization ahs already happened in the constructor
//-----------------------------------------------------------------------------
    if (list_of_pidp) {
      const mu2e::PIDProduct* pidp = &list_of_pidp->at(itrk);
      track->fEleLogLHDeDx = pidp->GetLogEProb();
      track->fMuoLogLHDeDx = pidp->GetLogMProb();
      track->fRSlope       = pidp->GetResidualsSlope();
      track->fRSlopeErr    = pidp->GetResidualsSlopeError();
    }

    track->fKalRep[0] = krep;
    mask = (0x0001 << 16) | 0x0000;

    if (list_of_algs) {
      alg_id = &list_of_algs->at(itrk);
      mask   = alg_id->BestID() | (alg_id->AlgMask() << 16);
    }

    track->fAlgorithmID = mask;
//-----------------------------------------------------------------------------
// in all cases define momentum at lowest Z - ideally, at the tracker entrance
//-----------------------------------------------------------------------------
    h1_fltlen      = krep->firstHit()->kalHit()->hitOnTrack()->fltLen() - 10;
    hn_fltlen      = krep->lastHit ()->kalHit()->hitOnTrack()->fltLen() - 10;
    entlen         = std::min(h1_fltlen,hn_fltlen);
    
    CLHEP::Hep3Vector fitmom = krep->momentum(entlen);
    HepPoint   pos    = krep->position(entlen);

    track->fX1 = pos.x();
    track->fY1 = pos.y();
    track->fZ1 = pos.z();

    double z2 = -1540.;

    double s2 = entlen*z2/track->fZ1;
    CLHEP::Hep3Vector fitmom2 = krep->momentum(s2);

    track->fP2 = fitmom2.mag();

    CLHEP::Hep3Vector momdir = fitmom.unit();
    
    BbrVectorErr      momerr = krep->momentumErr(entlen);
    
    HepVector momvec(3);
    for (int i=0; i<3; i++) momvec[i] = momdir[i];
    
    fitmom_err = sqrt(momerr.covMatrix().similarity(momvec));
    
    //  assuming electron mass, all - in MeV

    double px, py, pz;
    px = fitmom.x();
    py = fitmom.y();
    pz = fitmom.z();

    track->Momentum()->SetXYZM(px,py,pz,0.511);
    track->fNActive   = krep->nActive();
    track->fChi2      = krep->chisq();
    track->fFitCons   = krep->chisqConsistency().consistency();
    track->fT0        = krep->t0().t0();
    track->fT0Err     = krep->t0().t0Err();
    track->fTanDip    = krep->helix(0).tanDip();

    double curv = krep->helix(0).omega();
    if (curv > 0) track->fCharge = 1.;
    else          track->fCharge = -1;

    track->fFitMomErr = fitmom_err;
    track->fP         = track->Momentum()->P ();
    track->fPt        = track->Momentum()->Pt();
    track->fD0        = krep->helix(0).d0();
    track->fZ0        = krep->helix(0).z0();
//-----------------------------------------------------------------------------
// fP0 : track momentum at Z0
//-----------------------------------------------------------------------------
    double px0, py0, pz0;
    px0 = krep->momentum(0).x();
    py0 = krep->momentum(0).y();
    pz0 = krep->momentum(0).z();

    track->fP0 = sqrt(px0*px0+py0*py0+pz0*pz0);
//-----------------------------------------------------------------------------
    const mu2e::TrkStrawHit  *hit, *closest_hit;
    const TrkHotList*        hot_list = krep->hotList();
    
    double chi2c(0), dr;
    for(TrkHotList::hot_iterator it=hot_list->begin(); it<hot_list->end(); it++) {
      hit = (const mu2e::TrkStrawHit*) &(*it);
      dr  = hit->resid();
      //      Hist->fResid->Fill(dr);
      // normalize by 100 microns
      chi2c += dr*dr/0.01;
    }

    track->fChi2C = chi2c;

    //      id_word = fTrackID->IDWord(TMu2eStnTrackID = TrackPassedSelectionC(track);
    
    for (int j=0; j<40; j++) {
      track->fNHPerStation[j] = 0;
    }
    
    int     loc, ipart, nhits, n_straw_hits, found; // , pdg_code;
    int     id,  npart(0), part_pdg_code[100], part_nh[100], part_id[100];

    mu2e::StrawHit            *s_hit0;
    const mu2e::StrawHit      *s_hit; 
    const mu2e::SimParticle   *sim; 


    n_straw_hits = list_of_straw_hits->size();

    if (n_straw_hits <= 0) {
      printf(">>> ERROR in StntupleInitMu2eTrackBlock: wrong hit collection used, NHITS = %i\n",
	     n_straw_hits);
    }
    else {

      s_hit0 = &list_of_straw_hits->at(0);

      for(TrkHotList::hot_iterator it=hot_list->begin(); it<hot_list->end(); it++) {

	hit = (const mu2e::TrkStrawHit*) &(*it);

	s_hit = &hit->strawHit();
	loc = s_hit-s_hit0;
	if ((loc >= 0) && (loc < n_straw_hits)) {

	  if (list_of_mc_straw_hits) {
	    mu2e::PtrStepPointMCVector  const& mcptr(list_of_mc_straw_hits->at(loc));
	    int nj = mcptr.size();
					// 2013-06-16 P.Murat: this is a borrowed piece of code.
					// a loop below doesn't seem to be necessary
					// as everything is defined by the first hit...
	    for (int j=0; j<nj; j++) {
	      step     = &(*mcptr.at(j));
	      sim      = &(*step->simParticle());
	      if (sim != NULL) {
		id       = sim->id().asInt();
		//		pdg_code = sim->pdgId();
	      }
	      else {
		printf(">>> ERROR in %s : sim is NULL, set PDG_CODE to -1\n",oname);
		id       = -1;
		//		pdg_code = -1;
	      }
	      break;
	    }
	  }
	  
	  found = 0;
	  for (int ip=0; ip<npart; ip++) {
	    if (id == part_id[ip]) {
	      found        = 1;
	      part_nh[ip] += 1;
	      break;
	    }
	  }
	  
	  if (found == 0) {
	    part_id      [npart] = id;
	    part_pdg_code[npart] = sim->pdgId();
	    part_nh      [npart] = 1;
	    npart               += 1;
	  }
	}
	else {
	  printf(">>> ERROR in StntupleInitMu2eTrackBlock: wrong hit collection used, loc = %i\n",loc);
	}

	mu2e::Straw*   straw = (mu2e::Straw*) &hit->straw();
	
	const mu2e::StrawId& straw_id = straw->id();
	
	int ist = straw_id.getDevice();
	
	track->fNHPerStation[ist] += 1;
	
	int sec = straw_id.getSector();
	int lay = straw_id.getLayer ();
	
	int bit = zmap.fMap[ist][sec][lay];

	track->fHitMask.SetBit(bit,1);
      }
    }
//-----------------------------------------------------------------------------
// given track parameters, build the expected hit mask
//-----------------------------------------------------------------------------
    double z, closest_z, dz, zw, dz_min, s0, s;
    int    station, offset;
    int    nz = 88;
    for (int iz=0; iz<nz; iz++) {
      z = zmap.fZ[iz];
					// find the track hit closest to that Z

      dz_min = 1.e10;
      for(TrkHotList::hot_iterator it=hot_list->begin(); it<hot_list->end(); it++) {

	hit = (const mu2e::TrkStrawHit*) &(*it);
	
	s_hit = &hit->strawHit();
	loc = s_hit-s_hit0;

	zw = hit->straw().getMidPoint().z();

	dz = z-zw;

	if (fabs(dz) < dz_min) {
	  dz_min = fabs(dz);
	  closest_hit = hit;
	  closest_z   = zw;
	}
      }
//-----------------------------------------------------------------------------
// found closest hit and the extrapolation length, then extrapolate track
//-----------------------------------------------------------------------------
      s0  = closest_hit->fltLen();

      s   = (z-track->fZ0)/(closest_z-track->fZ0)*s0;

      HepPoint    pz    = krep->position(s);

      get_station(&zmap,z,&station,&offset);

      const mu2e::Sector*     sec0 = NULL;
      const mu2e::Sector*     sec;
      const mu2e::Device*     dev;
      double            min_dphi = 1.e10;
      double            dphi, nx, ny, wx, wy, wrho, rho, phi0;
      Hep3Vector        w0mid;
      int               isec;
      for (int i=0; i<3; i++) {
	isec = 2*i+offset;		// segment number
					// check if point pz(x0,y0) overlaps with the segment iseg
					// expected mask is set to zero
	dev   = &tracker->getDevice(station);
	sec   = &dev->getSector(isec);
	
	w0mid = sec->straw0MidPoint();
					// also calculate pho wrt the sector
	wx    = w0mid.x();
	wy    = w0mid.y();

	phi0  = w0mid.phi(); // make sure I understand the range

	wrho  = sqrt(wx*wx+wy*wy);

	nx    = wx/wrho;
	ny    = wy/wrho;

	rho = nx*pz.x()+ny*pz.y();

	dphi = TVector2::Phi_mpi_pi(phi0-pz.phi());
	if ((abs(dphi) < min_dphi) && (rho > wrho)) {
	  min_dphi = fabs(dphi);
	  sec0 = sec;
	}
      }
//-----------------------------------------------------------------------------
// OK, closest segment found, set the expected bit..
//-----------------------------------------------------------------------------
      if (sec0 != NULL) {
	track->fExpectedHitMask.SetBit(iz,1);
      }
    }
//-----------------------------------------------------------------------------
// identify track with the particle which produced most hits
//-----------------------------------------------------------------------------
    ipart = 0;
    nhits = part_nh[0];

    for (int ip=1; ip<npart; ip++) {
      if (part_nh[ip] > nhits) {
	nhits = part_nh[ip];
	ipart = ip;
      }
    }

    track->fPdgCode     = part_pdg_code[ipart];
    track->fPartID      = part_id      [ipart];
    track->fNGoodMcHits = nhits;
//-----------------------------------------------------------------------------
// particle parameters at virtual detectors
//-----------------------------------------------------------------------------
    mu2e::GeomHandle<mu2e::VirtualDetector> vdg;

    track->fPFront = -1.;
    track->fPStOut = -1.;

    if (vdg->nDet() > 0) {
      art::Handle<mu2e::StepPointMCCollection> vdhits;
      AnEvent->getByLabel(g4_module_label,"virtualdetector",vdhits);
      if (!vdhits.isValid()) {
	printf("ERROR %s : invalid VD step points\n",oname);
      }
      else {
	int nvdhits = vdhits->size();
	for (int i=0; i<nvdhits; i++) {
	  const mu2e::StepPointMC* hit = &(*vdhits)[i];
	  
	  //int vdid = hit.volumeId();
	  mu2e::VirtualDetectorId vdid(hit->volumeId());

	  if (vdid.id() == mu2e::VirtualDetectorId::ST_Out) {

	    //	    TAnaDump::Instance()->printStepPointMC(hit,"");

	    art::Ptr<mu2e::SimParticle> const& simptr = hit->simParticle();
	    const mu2e::SimParticle* sim  = simptr.operator ->();

	    if (sim == NULL) {
	      printf(">>> ERROR: %s sim == NULL\n",oname);
	    }
	    int sim_id = sim->id().asInt();
	    if (sim_id == track->fPartID) {
	      track->fPStOut = hit->momentum().mag();
	    }
	  }
	  else if (vdid.isTrackerFront()) {

	    //	    TAnaDump::Instance()->printStepPointMC(hit,"");

	    art::Ptr<mu2e::SimParticle> const& simptr = hit->simParticle();
	    const mu2e::SimParticle* sim  = simptr.operator ->();

	    if (sim == NULL) {
	      printf(">>> ERROR: %s sim == NULL\n",oname);
	    }
	    int sim_id = sim->id().asInt();
	    if (sim_id == track->fPartID) {
	      track->fPFront = hit->momentum().mag();
	    }
	  }
	  
	}
      }
    }
//-----------------------------------------------------------------------------
// number of true MC hits in the tracker
//-----------------------------------------------------------------------------
    const mu2e::PtrStepPointMCVectorCollection* stepPointMCVectorCollection;
    //    art::Handle<mu2e::StepPointMCCollection>    stepsHandle;
    //    const mu2e::StepPointMCCollection*          steps;
    const mu2e::StepPointMC*                    step;

    track->fNMcStrawHits = 0;

    art::Handle<mu2e::PtrStepPointMCVectorCollection> mcptrHandleStraw;
    AnEvent->getByLabel(strh_module_label,"StrawHitMCPtr",mcptrHandleStraw);
    stepPointMCVectorCollection = mcptrHandleStraw.product();

//     art::Selector getTrackerSteps(art::ProductInstanceNameSelector("tracker") &&
// 				  art::ProcessNameSelector("") &&
// 				  art::ModuleLabelSelector(g4_module_label)  );
//     AnEvent->get(getTrackerSteps, stepsHandle);
//     steps =  (const mu2e::StepPointMCCollection*) &(*stepsHandle);

    for (int i=0; i<n_straw_hits; i++) {
      //      const mu2e::StepPointMC* hit = &(*steps)[i];
	  
      mu2e::PtrStepPointMCVector const& mcptr(stepPointMCVectorCollection->at(i) );

      step = mcptr[0].operator ->();
    
      //      hit   = &list_of_straw_hits->at(i);

      art::Ptr<mu2e::SimParticle> const& simptr = step->simParticle(); 
      art::Ptr<mu2e::SimParticle> mother = simptr;
      while(mother->hasParent())  mother = mother->parent();
      const mu2e::SimParticle*    sim    = mother.operator ->();

      //      int pdg_id        = simptr->pdgId();
      //      int mother_pdg_id = sim->pdgId();

      int sim_id = sim->id().asInt();

      if (sim_id == track->fPartID) {
	track->fNMcStrawHits += 1;
      }
    }
//-----------------------------------------------------------------------------
// consider half-ready case when can't use the extrapolator yet
// turn it off softly
//-----------------------------------------------------------------------------
    const mu2e::TrkToCaloExtrapol* extrk;
    const KalRep *                 krep;
    Hep3Vector                     x1, x2;
    HepPoint                       extr_point;
    Hep3Vector                     extr_mom;
    int                            iv, next;
    TStnTrack::InterData_t*        vint;

    if (list_of_extrapolated_tracks != NULL) {
      next = list_of_extrapolated_tracks->size();
    }
    else {
      next = 0;
    }
  
    for (int iext=0; iext<next; iext++) {
      extrk = &list_of_extrapolated_tracks->at(iext);
      krep  = *extrk->trk();
      if (krep == track->fKalRep[0]) {
	if (track->fExtrk == 0) {
	  track->fExtrk = extrk;
	}
	if (bc) { 
//-----------------------------------------------------------------------------
// store coordinates of the best intersection in a plane
//-----------------------------------------------------------------------------
	  iv   = extrk->vaneId();
	  vint = &(track->fVane[iv]);

	  if (vint->fID == -1) {
	    vint->fID        = iv;
	    vint->fExtrk     = extrk;
	    vint->fChi2Match = 1.e6;
	  }
	  else {
	    printf("%s : ADDITIONAL EXTR POINT for track %i on vane = %i\n", oname,itrk,iv);
	  }
	}
      }
    }
//-----------------------------------------------------------------------------
// now loop over track-cluster matches and find the right ones to associate with 
// the track
//-----------------------------------------------------------------------------
    unsigned int nm (0);

    const mu2e::TrackClusterMatchCollection* tcmcoll = tcmH.product();
    
    if (tcmcoll != NULL) nm = tcmcoll->size();

    const mu2e::TrackClusterMatch* tcm;

    double best_chi2_match(1.e6);

    for (size_t im=0; im<nm; im++) {
      tcm   = &tcmcoll->at(im);
      extrk = tcm->textrapol();
      krep  = *extrk->trk();
      if (krep == track->fKalRep[0]) {
	const mu2e::CaloCluster* cl = tcm->caloCluster();
	iv   = cl->vaneId();
	vint = &track->fVane[iv];
	if (bc == 0) {
	  printf(">>> ERROR: %s VANE calorimeter is not defined \n",oname);
	  continue;
	}

	x1   = bc->toSectionFrame(iv,cl->cog3Vector());

	if ((track->fClosestCluster == NULL) || (tcm->chi2() < best_chi2_match )) {
//-----------------------------------------------------------------------------
// if closest cluster has not been defined or the energy of the new one is higher
// depending on the calorimeter geometry choice either DX or DZ is meaningful
//-----------------------------------------------------------------------------
	  track->fClosestCluster = cl;
	  track->fExtrk          = extrk;
	  best_chi2_match        = tcm->chi2();
	}

	vint->fXTrk = tcm->xtrk();
	vint->fYTrk = tcm->ytrk();
	vint->fZTrk = tcm->ztrk();
	
	vint->fNxTrk = tcm->nx();
	vint->fNyTrk = tcm->ny();
	vint->fNzTrk = tcm->nz();

	if (vint->fCluster == 0) {
	  vint->fCluster   = cl;
	  vint->fEnergy    = cl->energyDep();
	  vint->fXCl       = x1.x();
	  vint->fYCl       = x1.y();
	  vint->fZCl       = x1.z();
	  vint->fDt        = tcm->dt();
	  vint->fDx        = tcm->du();
	  vint->fDy        = tcm->dv();
	  vint->fDz        = tcm->dz();
	  vint->fChi2Match = tcm->chi2()-tcm->chi2_time();
	  vint->fPath      = extrk->pathLengthExit()-extrk->pathLengthEntrance();
	}
	else {
	  printf("%s : ADDITIONAL MATCH for track %i on vane = %i\n", oname,itrk,iv);
	}
      }
    }
//-----------------------------------------------------------------------------
// find intersections to use for electron ID, 
// in this version both VMinS and VMaxEp are the same
//-----------------------------------------------------------------------------
    double                    min_chi2_match(1.e6);
    TStnTrack::InterData_t*   v;

    track->fVMinS  = 0;
    track->fVMaxEp = 0;

    for (int iv=0; iv<4; iv++) {
      v = &track->fVane[iv];
      if (v->fID >= 0) {
	if (v->fCluster) {
	  if (v->fChi2Match < min_chi2_match) {
	    track->fVMaxEp = v;
	    track->fVMinS  = v;
	    min_chi2_match = v->fChi2Match;
	  }
	}
      }
    }
//-----------------------------------------------------------------------------
// define E/P by the first intersection, if it exists, the second in the 
// high-occupancy environment may be unreliable
//-----------------------------------------------------------------------------
    track->fClusterE = -track->fP;
    track->fDt       = -1.e12;
    track->fDx       = -1.e12;
    track->fDy       = -1.e12;
    track->fDz       = -1.e12;

    if (track->fVMinS != 0) {
      if (track->fVMinS->fCluster) {
	track->fClusterE = track->fVMinS->fCluster->energyDep();
	track->fDx       = track->fVMinS->fDx;
	track->fDy       = track->fVMinS->fDy;
	track->fDz       = track->fVMinS->fDz;
	track->fDt       = track->fVMinS->fDt;
      }
      else {
//-----------------------------------------------------------------------------
// intersection with minimal S doesn't have a cluster, check MaxP
//-----------------------------------------------------------------------------
	if (track->fVMaxEp != 0) {
	  if (track->fVMaxEp->fCluster) {
	    track->fClusterE = track->fVMaxEp->fCluster->energyDep();
	    track->fDx       = track->fVMaxEp->fDx;
	    track->fDy       = track->fVMaxEp->fDy;
	    track->fDz       = track->fVMaxEp->fDz;
	    track->fDt       = track->fVMaxEp->fDt;
	  }
	}
      }
    }

    track->fEp = track->fClusterE/track->fP;
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
  }
					// on return set event and run numbers
					// to mark block as initialized
  data->f_RunNumber   = rn_number;
  data->f_EventNumber = ev_number;

  return 0;
}

//_____________________________________________________________________________
Int_t StntupleInitMu2eTrackBlockLinks(TStnDataBlock* Block, AbsEvent* AnEvent, int Mode) 
{
  // Mu2e version, do nothing

  Int_t  ev_number, rn_number;

  ev_number = AnEvent->event();
  rn_number = AnEvent->run();

  if (! Block->Initialized(ev_number,rn_number)) return -1;

					// do not do initialize links 2nd time

  if (Block->LinksInitialized()) return 0;

  TStnTrackBlock* header = (TStnTrackBlock*) Block;
  //  TStnEvent* ev   = header->GetEvent();
//-----------------------------------------------------------------------------
// mark links as initialized
//-----------------------------------------------------------------------------
  header->fLinksInitialized = 1;

  return 0;
}
