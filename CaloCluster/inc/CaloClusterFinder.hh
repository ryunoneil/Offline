//
// Original author B.Echenard
//

#ifndef CaloCluster_CaloClusterFinder_HH_
#define CaloCluster_CaloClusterFinder_HH_


// Mu2e includes
#include "RecoDataProducts/inc/CaloCrystalHit.hh"
#include "CalorimeterGeom/inc/Calorimeter.hh"


// C++ includes
#include <unordered_map>
#include <map>
#include <queue>



namespace mu2e {


    class CaloClusterFinder {


	 public:
             
	     typedef std::list<CaloCrystalHit const*>    CaloCrystalList;
	     typedef std::vector<CaloCrystalHit const*>  CaloCrystalVec;


	     CaloClusterFinder(Calorimeter const& cal, CaloCrystalHit const& crystalSeed, double deltaTimePlus, double deltaTimeMinus, double ExpandCut);              
	     ~CaloClusterFinder(){};
	     
	     
	     CaloCrystalList const& clusterList()  const {return _clusterList;}
	     CaloCrystalList const& inspected()    const {return _inspected;}
	     
             void formCluster(std::vector<CaloCrystalVec>& idHitVec);


	 private:
             
             Calorimeter const*     _cal;
	     CaloCrystalHit const*  _crystalSeed;
	     double                 _seedTime;

	     CaloCrystalList        _clusterList;
             CaloCrystalList        _inspected;           
	     std::queue<int>        _crystalToVisit;
             std::vector<bool>      _isVisited; 

	     double                 _deltaTimePlus; 
	     double                 _deltaTimeMinus; 
	     double                 _ExpandCut;

    };


}

#endif
