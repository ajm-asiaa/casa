// -*- C++ -*-
//# CTMainColInterface.h: The generic interface for tables that can be used with MSSelection
//# Copyright (C) 1996,1997,1998,1999,2001
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
//#                        Charlottesville, VA 22903-2475 USA
//#
//#
//# $Id$

#ifndef CT_CTMAINCOLINTERFACE_H
#define CT_CTMAINCOLINTERFACE_H

#include <casa/aips.h>
#include <casa/BasicSL/String.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Cube.h>
#include <casa/Containers/OrderedMap.h>
#include <casa/Containers/MapIO.h>
#include <tables/Tables/ExprNode.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <ms/MeasurementSets/MSMainEnums.h>
#include <ms/MeasurementSets/MSSelectionError.h>
#include <ms/MeasurementSets/MSSelectableTable.h>
#include <synthesis/CalTables/CTMainColumns.h>
namespace casa { //# NAMESPACE CASA - BEGIN


  class CTMainColInterface: public MSSelectableMainColumn
  {
  public: 
    CTMainColInterface():MSSelectableMainColumn() {};
    CTMainColInterface(const Table& ctAsTable): MSSelectableMainColumn(ctAsTable)
    {init(ctAsTable);}

    virtual ~CTMainColInterface() {delete ctCols_p;};

    virtual void init(const Table& ctAsTable)
    {MSSelectableMainColumn::init(ctAsTable);ct_p = NewCalTable(ctAsTable); ctCols_p=new ROCTMainColumns(ct_p);}

    virtual const ROArrayColumn<Bool>& flag() {return ctCols_p->flag();}

    virtual const Bool flagRow(const Int& i) {return allTrue(ctCols_p->flag()(i));}
    //virtual const Bool flagRow(const Int& i) {return msCols_p->flagRow()(i);}
    
    // For now, return timeEPQuant() even exposureQuant.
    virtual const ROScalarQuantColumn<Double>& exposureQuant() {return ctCols_p->timeEPQuant();};
    virtual const ROScalarQuantColumn<Double>& timeQuant()     {return ctCols_p->timeEPQuant();}

    // This is not an MS
    virtual const MeasurementSet *asMS(){return NULL;};
  private:
    NewCalTable ct_p;
    ROCTMainColumns *ctCols_p;

  };

} //# NAMESPACE CASA - END

#endif
