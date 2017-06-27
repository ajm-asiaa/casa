//# SynthesisUtilMethods.h: Imager functionality sits here; 
//# Copyright (C) 1996,1997,1998,1999,2000,2001,2002,2003
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#
//# $Id$

#ifndef SYNTHESIS_SYNTHESISUTILS_H
#define SYNTHESIS_SYNTHESISUTILS_H

#include <casa/aips.h>
#include <casa/OS/Timer.h>
#include <casa/Containers/Record.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <casa/Arrays/IPosition.h>
#include <casa/Quanta/Quantum.h>
#include <casa/Quanta/QuantumHolder.h>
#include <measures/Measures/MDirection.h>
#include <casa/Quanta/MVAngle.h>
#include <measures/Measures/MFrequency.h>
#include <casa/Utilities/DataType.h>
#include <stdcasa/StdCasa/CasacSupport.h>
#include <coordinates/Coordinates/Projection.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <scimath/Mathematics/GaussianBeam.h>

#include <msvis/MSVis/VisibilityIterator.h>
#include <msvis/MSVis/VisibilityIterator2.h>
//#include <synthesis/ImagerObjects/TmpSwitch.h>

namespace casa { //# NAMESPACE CASA - BEGIN

  class SynthesisParams;
  class SynthesisParamsImage;
  class SynthesisParamsSelect;
  
  class SynthesisUtilMethods
  {
  public:
    enum VBSTATES {NOVALIDROWS=-1};
    // Default constructor

    SynthesisUtilMethods();
    ~SynthesisUtilMethods();
    
    // All functions here are stand-alone, self-contained methods.
    
    
    // Partitioning syntax for Selection parameters
    //
    // casacore::Input casacore::Record (example) :
    //  { 'ms0' : { 'msname':xxx1, 'spw':yyy } ,
    //     'ms1' : { 'msname':xxx2, 'spw':yyy } }
    //
    //  Output casacore::Record (example for partitioning on spw) : 
    //  { '0' : { 'ms0' : { 'msname':xxx1, 'spw':yyy1 } ,
    //              'ms1' : { 'msname':xxx2, 'spw':yyy1 }    }
    //    '1' : { 'ms0' : { 'msname':xxx1, 'spw':yyy2 } ,
    //               'ms1' : { 'msname':xxx2, 'spw':yyy2 }   }   }
    
    // casacore::Data partitioning rules for CONTINUUM imaging
    casacore::Record continuumDataPartition(casacore::Record &selpars, const casacore::Int npart);
    
    // casacore::Data partitioning rules for CUBE imaging
    //uniform contiguous partition in frequency step
    // Note that the spw selection will have a "-1"
    //if there is no overlap in the data selection provided and 
    // frequency range provided
    static casacore::Record cubeDataPartition(const casacore::Record &selpars, const casacore::Int npart, const casacore::Double freqBeg, const casacore::Double freqEnd, const casacore::MFrequency::Types eltype=casacore::MFrequency::LSRK);

    // freqBeg and freqEnd are frequency range  of the sub image cubes defined in frame set here
    // number of partions is obviously the length of freqBeg and freqEnd 
    // Use this for non uniform width of imge frequencies
    static casacore::Record cubeDataPartition(const casacore::Record& selpars, const casacore::Vector<casacore::Double>& freqBeg, const casacore::Vector<casacore::Double>& freqEnd, const casacore::MFrequency::Types frame=casacore::MFrequency::LSRK);
    
    //casacore::CoordinateSystem and number of channels of original cube is passed in 
    //Output record is the ms's and data selection for each part.
    // also the casacore::Vector of outCsys and outnChan are the  coordinatesystems
    // and nchannel of the sub cube for each part.
    // The image is divided  in n part along spectral channel
    //now if one of the sub cube has no match the  the spw selection will have a "-1"
    //for that part. The caller will have to deal with that for load balancing etc..
    //  Output casacore::Record (example for partitioning on spw) : 
    //  { '0' : { 'ms0' : { 'msname':xxx1, 'spw': '0:5~10' } ,
    //              'ms1' : { 'msname':xxx2, 'spw':'0:20~25' },   
    //                 'nchan': 6, 
    //                 'coordsys': { A record of the coordinatesystem of subcube 0}}
    //    '1' : { 'ms0' : { 'msname':xxx1, 'spw':'0:9~14' } ,
    //               'ms1' : { 'msname':xxx2, 'spw':'0:24~29' },
    //               'nchan':6, 
    //                'coordsys': { A record of the coordinatesystem of subcube 1} }
    //   }
    // the coordsys keyed record can be use in casacore::CoordinateSystem:;restore to reconstrucr
    // it is the same as the parameter outCsys
    static casacore::Record cubeDataImagePartition(const casacore::Record & selpars, const casacore::CoordinateSystem&
				    incsys, const casacore::Int npart, const casacore::Int nchannel, 
				    casacore::Vector<casacore::CoordinateSystem>& outCsys,
				    casacore::Vector<casacore::Int>& outnChan);

    // Image cube partitioning rules for CUBE imaging
    casacore::Record cubeImagePartition(casacore::Record &impars, casacore::Int npart);
    
    // Validate the supplied VB.  This currently only checks for the
    // existence of at least one non-flagged row and returns the index
    // of the first non-flagged row found. Otherwise returns
    // SynthesisUtilMethods::NOVALIDROWS. This static method is called
    // in the vis-iter loops to block invalid VBs from flowing down
    // the pipes.
    static casacore::Int validate(const VisBuffer& vb);
    static casacore::Int validate(const vi::VisBuffer2& vb);


    static casacore::Int getOptimumSize(const casacore::Int npix);

    static casacore::Int parseLine(char* line);
    static void getResource(casacore::String label="",casacore::String fname="");
    
  protected:
    static casacore::String mergeSpwSel(const casacore::Vector<casacore::Int>& fspw, const casacore::Vector<casacore::Int>& fstart, const casacore::Vector<casacore::Int>& fnchan, const casacore::Matrix<casacore::Int>& spwsel);

    static casacore::Vector<casacore::uInt> primeFactors(casacore::uInt n, casacore::Bool douniq=true);

  };

class SynthesisParams
{
public:
  SynthesisParams(){};
  virtual ~SynthesisParams(){};
  ///As there is no state to copy
  virtual SynthesisParams& operator= (const SynthesisParams& /*other*/){ return *this;};
  virtual void fromRecord(const casacore::Record &inrec)=0;
  virtual void setDefaults()=0;
  virtual casacore::String verify()const =0 ;
  virtual casacore::Record toRecord() const =0;
protected:
  // All return strings are error messages. Null if valid.
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::String& val) const ;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::Int& val) const ;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::Float& val) const;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::Bool& val) const ;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::Vector<casacore::Int>& val) const;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::Vector<casacore::Float>& val) const ;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::Vector<casacore::String>& val) const ;
  casacore::String stringToQuantity(casacore::String instr, casacore::Quantity& qa) const;
  casacore::String stringToMDirection(casacore::String instr, casacore::MDirection& md) const ;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::Quantity& val) const;
  casacore::String readVal(const casacore::Record &rec, casacore::String id, casacore::MDirection& val) const ;
  // Others..
  casacore::String MDirectionToString(casacore::MDirection val) const;
  casacore::String QuantityToString(casacore::Quantity val) const;
  casacore::String recordQMToString(const casacore::Record &rec) const ;
};

  class SynthesisParamsSelect : public SynthesisParams
{
public:

  SynthesisParamsSelect();
  SynthesisParamsSelect(const SynthesisParamsSelect& other);
  ~SynthesisParamsSelect();

  //copy semantics
  virtual SynthesisParamsSelect& operator=(const SynthesisParamsSelect& other);
  void fromRecord(const casacore::Record &inrec);
  void setDefaults();
  casacore::String verify() const;
  casacore::Record toRecord() const;

  casacore::String msname, spw, freqbeg, freqend;
  casacore::MFrequency::Types freqframe;
  casacore::String field, antenna, timestr, scan, obs, state, uvdist,taql,intent;
  casacore::Bool usescratch,readonly,incrmodel;

  casacore::String datacolumn;

};


 
  class SynthesisParamsImage: public SynthesisParams
{
public:

  SynthesisParamsImage();
  ~SynthesisParamsImage();

  void fromRecord(const casacore::Record &inrec);
  void setDefaults();
  casacore::String verify() const;
  casacore::Record toRecord()const ;

  // Generate casacore::Coordinate System 
  casacore::CoordinateSystem buildCoordinateSystem(ROVisibilityIterator* rvi);
  casacore::CoordinateSystem buildCoordinateSystem(vi::VisibilityIterator2& vi2, const std::map<casacore::Int, std::map<casacore::Int, casacore::Vector<casacore::Int> > >& chansel );
  casacore::CoordinateSystem buildCoordinateSystemCore(casacore::MeasurementSet& msobj, 
					     casacore::Vector<casacore::Int> spwids, casacore::Int fld, 
					     casacore::Double freqmin, casacore::Double freqmax, 
                                             casacore::Double datafstart, casacore::Double datafend);

  casacore::Vector<casacore::Int> decideNPolPlanes(const casacore::String& stokes) const;
  casacore::IPosition shp() const;
  casacore::Bool getImFreq(casacore::Vector<casacore::Double>& ChanFreq, casacore::Vector<casacore::Double>& ChanWidth, 
		 casacore::Double& refPix, casacore::String& specmode,
		 const casacore::MEpoch& obsEpoch, const casacore::MPosition& obsPosition,
		 const casacore::Vector<casacore::Double>& dataChanFreqs, const casacore::Vector<casacore::Double>& dataFreqRes,
		 const casacore::MFrequency::Types& dataFrame, const casacore::Quantity& qrestfreq, 
		 const casacore::Double& freqmin, const casacore::Double& freqmax,
		 const casacore::MDirection& phaseCenter );
  
  casacore::String findSpecMode(const casacore::String& mode) const;
  casacore::String MDopToVelString(casacore::Record &rec);
  casacore::Record getcsys() const;
  // check consistency of image parameters when csys record exists and update 
  // accordingly based on csys record 
  casacore::Record updateParams(const casacore::Record &impar);

  // Sky coordinates
  casacore::String imageName, stokes;
  casacore::Vector<casacore::String> startModel;
  casacore::Vector<casacore::Int> imsize;
  casacore::Vector<casacore::Quantity> cellsize;
  casacore::Projection projection;
  casacore::Bool useNCP;
  casacore::MDirection phaseCenter;
  casacore::Int phaseCenterFieldId;

  // Spectral coordinates ( TT : Add other params here  )
  casacore::Int nchan, nTaylorTerms, chanStart, chanStep;
  casacore::Quantity freqStart, freqStep, refFreq, velStart, velStep;
  casacore::MFrequency::Types freqFrame;
  casacore::MFrequency mFreqStart, mFreqStep;
  casacore::MRadialVelocity mVelStart, mVelStep;
  casacore::Vector<casacore::Quantity> restFreq;
  casacore::String start, step, frame, veltype, mode, reffreq, sysvel, sysvelframe;
  // private variable to store ref frame defined in casacore::Quantity or casacore::Measure 
  // in start or step parameters and veltype from measure (e.g. casacore::MDoppler)
  casacore::String qmframe, mveltype;
  casacore::String tststr;
  // for holding quantity or measure records
  casacore::Record startRecord, stepRecord, reffreqRecord, sysvelRecord, restfreqRecord;
  // optional coordsys record
  casacore::Record csysRecord, csys;
  casacore::Vector<casacore::Int> imshape;

  //freqframe coversion?
  casacore::Bool freqFrameValid;

  casacore::Bool overwrite;

  casacore::String deconvolver;

};


  class SynthesisParamsGrid: public SynthesisParams
{
public:

  SynthesisParamsGrid();
  ~SynthesisParamsGrid();

  void fromRecord(const casacore::Record &inrec);
  void setDefaults();
  casacore::String verify() const;
  casacore::Record toRecord() const;

  casacore::String imageName;

  // FTMachine setup
  casacore::String gridder, ftmachine, convFunc, vpTable;
  casacore::Int wprojplanes;
  casacore::Bool useDoublePrec, useAutoCorr; 
  casacore::Float padding;

  // Facets for gridding.
  casacore::Int facets;

  // casacore::Cube chunks for gridding
  casacore::Int chanchunks;

  // Spectral axis interpolation
  casacore::String interpolation;

  // Moving phase center ? 
  casacore::Quantity distance;
  casacore::MDirection trackDir;
  casacore::Bool trackSource; 
  
  // For wb-aprojection ftm.
  casacore::Bool aTermOn, psTermOn,mTermOn,wbAWP,doPointing, doPBCorr, conjBeams;
  casacore::String cfCache;
  casacore::Float computePAStep, rotatePAStep;

  // Mapper Type.
  casacore::String mType;

};


  class SynthesisParamsDeconv: public SynthesisParams
{
public:

  SynthesisParamsDeconv();
  ~SynthesisParamsDeconv();

  void fromRecord(const casacore::Record &inrec);
  void setDefaults();
  casacore::String verify() const;
  casacore::Record toRecord() const;

  casacore::String imageName, algorithm;
  casacore::Vector<casacore::String> startModel;
  casacore::Int deconvolverId; // maybe remove ? It's only to tag summary info.
  casacore::Int nTaylorTerms; 
  casacore::Vector<casacore::Float> scales;
  casacore::Float scalebias;
  casacore::String maskString;
  casacore::String maskType;
  casacore::Vector<casacore::String> maskList;
  casacore::String autoMaskAlgorithm;
  casacore::Float pbMask;
  casacore::String maskThreshold;
  casacore::String maskResolution;
  casacore::Float fracOfPeak;
  casacore::Float maskResByBeam;
  casacore::Float sidelobeThreshold;
  casacore::Float noiseThreshold;
  casacore::Float lowNoiseThreshold;
  casacore::Float smoothFactor;
  casacore::Float minBeamFrac;
  casacore::Float cutThreshold;
  int nMask;
  bool autoAdjust;


 
  casacore::Bool interactive;

  casacore::GaussianBeam restoringbeam;
  casacore::String usebeam;

  };


} //# NAMESPACE CASA - END

#endif
