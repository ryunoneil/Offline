// AlignmentMap.cc
// This is the definitions file for the AlignmentMap class.
// This class holds a map from Geometry element name strings to 
// corresponding AlignmentObj's
// for those elements that have them.
// David Norvil Brown, Oct 2017

#include "Alignment/inc/AlignmentMap.hh"
#include "Alignment/inc/AlignmentObj.hh"
#include "ConfigTools/inc/SimpleConfig.hh"

#include "cetlib_except/exception.h"

#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <unordered_map>

namespace mu2e {

  AlignmentMap::AlignmentMap() {
    _map.reserve(1024);
  }

  void AlignmentMap::make( const SimpleConfig& _config ) {

    // Get all the variable names found
    std::vector<std::string> theNames; 
    _config.getNames( theNames );

    // Make a list of the objects with Alignments specified
    std::vector<std::string> alignDets;
    for ( auto aVar : theNames ) {
      if ( aVar.find("translate") != std::string::npos ) { // has translate
	// process further
	// First check for right number of parts to variable name...
	std::size_t where1 = aVar.find(".");
	std::size_t where2;
	if ( where1 != std::string::npos ) {  // good...
	  // check for second .
	  where2 = aVar.find(".",where1+1);
	  if ( where2 == std::string::npos ) { //good...
	    // *** NOTE:  Original specification called for an iov 
	    // specification on each line.  No longer, so this part out ***
	    // // See if there is third
	    // std::size_t where3 = aVar.find(".",where2+1);
	    // if ( where3 != std::string::npos ) { // bad
	    //   throw cet::exception("ALIGN") <<
	    // 	"Badly configured Alignment specification: " << aVar << "\n";
	    // } 
	    //	  } else { // bad - no second dot
	    throw cet::exception("ALIGN") <<
	      "Badly configured Alignment specification: " << aVar << "\n";
	  }
	}  else { // bad - no first dot
	  throw cet::exception("ALIGN") <<
	    "Badly configured Alignment specification: " << aVar << "\n";
	}  
	std::string detec = aVar.substr(0,where1);
	alignDets.push_back(detec);

      }
    } // end of loop to make list of Aligned objects

    if ( alignDets.size() == 0 ) {
      throw cet::exception("ALIGN") <<
	"hasAlignment set true but no valid alignments specified!" << "\n";
    }

    // Now build the map
    unsigned int vno = 1;
    std::string prevDet = "";
    AlignmentObj* tmpAlignObj = 0; //new AlignmentSequence();

    for ( auto aVar : alignDets ) {
      if ( aVar == prevDet ) {
	vno++;
      } else {
	if ( prevDet != "" ) { 
	  vno = 1;
	  addAlignment(prevDet,*tmpAlignObj);
	  delete tmpAlignObj;
	  tmpAlignObj = 0;
	}
      }
      prevDet = aVar;
      std::ostringstream trname;
      trname << aVar << ".translate";
      std::ostringstream roname;
      roname << aVar << ".rotate";
      std::ostringstream stname;
      stname << aVar << ".startTime";
      std::ostringstream etname;
      etname << aVar << ".endTime";
      CLHEP::Hep3Vector theTranslate = _config.getHep3Vector(trname.str());
      std::vector<double> theRotVec;
      _config.getVectorDouble(roname.str(), theRotVec, 3 );
      CLHEP::HepRotation myRotate(CLHEP::HepRotation::IDENTITY);
      myRotate = myRotate.set( theRotVec[0],theRotVec[1],theRotVec[2]); // angles
      // No longer using IoVs
      //      IoV myIoV(_config.getDouble(stname.str()),_config.getDouble(etname.str()));
      tmpAlignObj = new AlignmentObj(theTranslate,myRotate);
      //      tmpAlignSeq->addPair(myIoV,tmpAO);
    } // end of loop over Dets to make map 
    // add the last one
    addAlignment(prevDet,*tmpAlignObj);
  } // end of make function

  AlignmentObj AlignmentMap::find( std::string& handle)  {
    //    std::cout << "The handle is: " << handle << std::endl;
    auto search = _map.find(handle);
    if ( search != _map.end() ) {
      AlignmentObj tmpObj( _map[handle]);
      return tmpObj;
    }
    AlignmentObj tmpObj2;
    return tmpObj2;
  } // end of find

  void AlignmentMap::addAlignment(std::string& handle, AlignmentObj& as) {
    //    std::string myHandle(handle);
    //    AlignmentSequence myAS(as);
    _map.emplace(handle,as);
  }// end of addAlignment function def

} // end of namespace mu2e
