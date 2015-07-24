//# LayeredVi2Factory.cc: Implementation of the LayeredVi2Factory class.
//#
//#  CASA - Common Astronomy Software Applications (http://casa.nrao.edu/)
//#  Copyright (C) Associated Universities, Inc. Washington DC, USA 2011, All rights reserved.
//#  Copyright (C) European Southern Observatory, 2011, All rights reserved.
//#
//#  This library is free software; you can redistribute it and/or
//#  modify it under the terms of the GNU Lesser General Public
//#  License as published by the Free software Foundation; either
//#  version 2.1 of the License, or (at your option) any later version.
//#
//#  This library is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY, without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//#  Lesser General Public License for more details.
//#
//#  You should have received a copy of the GNU Lesser General Public
//#  License along with this library; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
//#  MA 02111-1307  USA
//# $Id: $

#include <msvis/MSVis/LayeredVi2Factory.h>


#include <msvis/MSVis/VisibilityIterator2.h>
#include <msvis/MSVis/VisibilityIteratorImpl2.h>
#include <msvis/MSVis/VisBuffer2.h>
#include <msvis/MSVis/AveragingTvi2.h>
#include <msvis/MSVis/IteratingParameters.h>
#include <msvis/MSVis/AveragingVi2Factory.h>
#include <msvis/MSVis/CalibratingVi2FactoryI.h>
#include <msvis/MSVis/UtilJ.h>
#include <casa/BasicSL/String.h>

namespace casa { //# NAMESPACE CASA - BEGIN
namespace vi { //# NAMESPACE VI - BEGIN


// -----------------------------------------------------------------------
LayeredVi2Factory::LayeredVi2Factory(MeasurementSet* ms,
				     IteratingParameters* iterpar,
				     AveragingParameters* avepar) :
  ms_p(ms),
  iterpar_p(iterpar),
  avepar_p(avepar),
  doCal_p(False),
  callib_p(""),
  calrec_p(),  
  nlayer_p(1),
  calvi2factory_p(0)
{

  // Count requested layers
  if (avepar_p) ++nlayer_p;

}
// -----------------------------------------------------------------------
LayeredVi2Factory::LayeredVi2Factory(MeasurementSet* ms,
				     IteratingParameters* iterpar,
				     const Record& calrec,
				     AveragingParameters* avepar) :
  ms_p(ms),
  iterpar_p(iterpar),
  avepar_p(avepar),
  doCal_p(False),
  callib_p(""),
  calrec_p(calrec),  // This ctor, by Record _only_
  nlayer_p(1),
  calvi2factory_p(0)
{

  // Count requested layers
  if (avepar_p) ++nlayer_p;

  // ...and arrange for calibration, if necessary
  if (calrec_p.nfields()>0) {
    ++nlayer_p;  
    doCal_p=True;

    // Set up the CalibratingVi2Factory (via Record)
    calvi2factory_p = CalibratingVi2FactoryI::generate();
    calvi2factory_p->initialize(ms,calrec);
  }

}

// -----------------------------------------------------------------------
LayeredVi2Factory::LayeredVi2Factory(MeasurementSet* ms,
				     IteratingParameters* iterpar,
				     const String& callib,
				     AveragingParameters* avepar) :
  ms_p(ms),
  iterpar_p(iterpar),
  avepar_p(avepar),
  doCal_p(False),
  callib_p(callib),    // This ctor, by String _only_
  calrec_p(),
  nlayer_p(1),
  calvi2factory_p(0)
{

  // Count requested layers
  if (avepar_p) ++nlayer_p;


  if (callib_p.length()>0) {
    ++nlayer_p;  
    doCal_p=True;  // Calibration is turned on

    // Set up the CalibratingVi2Factory (via String)
    calvi2factory_p = CalibratingVi2FactoryI::generate();
    calvi2factory_p->initialize(ms,callib_p);
  }

}

// -----------------------------------------------------------------------
LayeredVi2Factory::~LayeredVi2Factory() {

  // Delete the CalibratingVi2FactorI*, if present
  if (calvi2factory_p)
    delete calvi2factory_p;

}


// -----------------------------------------------------------------------
vi::ViImplementation2 * LayeredVi2Factory::createVi (vi::VisibilityIterator2 * vi2) const
{

  //  cout << "MSVis::LayeredVi2Factor::createVi" << endl;


  // A Vector of Vii's to fill up and delegate, will return the last one
  Vector<vi::ViImplementation2*> viis(nlayer_p);

  // The bottom layer is the standard vii2 that does I/O
  Int ilayer(0);
  viis[ilayer]= new vi::VisibilityIteratorImpl2 (vi2,
						 Block<const MeasurementSet*>(1,ms_p),
						 iterpar_p->getSortColumns(),
						 iterpar_p->getChunkInterval(),
						 vi::VbPlain,
						 True); // writable!  (hardwired?)
  
  // TBD: consider if this is the layer where weight scaling should be applied?
  viis[ilayer]->setWeightScaling(iterpar_p->getWeightScaling());

  // If calibration requested
  if (doCal_p) {
    ++ilayer;
    Assert(ilayer<nlayer_p);
    // Call the factory
    viis[ilayer] = calvi2factory_p->createVi(vi2,viis[ilayer-1]);
  }

  // If (time) averaging requested
  if (avepar_p) {
    ++ilayer;
    Assert(ilayer<nlayer_p);
    // TBD: update the AveragingVi2Factory to permit layered createVi...
    viis[ilayer] = new AveragingTvi2(vi2,viis[ilayer-1],*avepar_p);
  }

  // Must be at the last layer now
  Assert((nlayer_p-ilayer)==1)
  Assert(viis[nlayer_p-1]);

  // Return outermost ViImplementation2 layer
  return viis[nlayer_p-1];

}

} //# NAMESPACE VI - END
} //# NAMESPACE CASA - END


