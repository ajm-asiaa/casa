/*
 * SDDoubleCircleGainCal.cpp
 *
 *  Created on: Jun 3, 2016
 *      Author: nakazato
 */

#include <synthesis/MeasurementComponents/SDDoubleCircleGainCal.h>
#include <synthesis/MeasurementComponents/StandardVisCal.h>
#include <synthesis/MeasurementComponents/SolvableVisCal.h>
#include <synthesis/MeasurementComponents/SDDoubleCircleGainCalImpl.h>
#include <synthesis/MeasurementEquations/VisEquation.h>
#include <synthesis/Utilities/PointingDirectionCalculator.h>
#include <msvis/MSVis/VisSet.h>

#include <casacore/casa/BasicSL/String.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/ArrayIO.h>
#include <casacore/casa/Quanta/Quantum.h>
#include <casacore/tables/TaQL/TableParse.h>
#include <casacore/measures/TableMeasures/ScalarQuantColumn.h>
#include <casacore/measures/TableMeasures/ArrayQuantColumn.h>
#include <casacore/ms/MeasurementSets/MSIter.h>

#include <iostream>
#include <sstream>
#include <map>

// Debug Message Handling
// if SDGAIN_DEBUG is defined, the macro debuglog and
// debugpost point standard output stream (std::cout and
// std::endl so that debug messages are sent to standard
// output. Otherwise, these macros basically does nothing.
// "Do nothing" behavior is implemented in NullLogger
// and its associating << operator below.
//
// Usage:
// Similar to standard output stream.
//
//   debuglog << "Any message" << any_value << debugpost;
//
#define SDGAINDBLC_DEBUG

namespace {
struct NullLogger {
};

template<class T>
inline NullLogger &operator<<(NullLogger &logger, T /*value*/) {
  return logger;
}
}

#ifdef SDGAINDBLC_DEBUG
#define debuglog std::cerr
#define debugpost std::endl
#else
::NullLogger nulllogger;
#define debuglog nulllogger
#define debugpost 0
#endif

namespace {
inline void doSomething() {
  return;
}

inline void fillNChanParList(casacore::String const &msName,
casacore::Vector<casacore::Int> &nChanParList) {
  casacore::MeasurementSet const ms(msName);
  casacore::MSSpectralWindow const &msspw = ms.spectralWindow();
  casacore::ROScalarColumn<casa::Int> nchanCol(msspw, "NUM_CHAN");
  debuglog<< "nchanCol=" << nchanCol.getColumn() << debugpost;
  nChanParList = nchanCol.getColumn()(casa::Slice(0, nChanParList.nelements()));
}

template<class T>
inline casacore::String toString(casacore::Vector<T> const &v) {
  std::ostringstream oss;
  oss << "[";
  std::string delimiter = "";
  for (size_t i = 0; i < v.nelements(); ++i) {
    oss << delimiter << v[i];
    delimiter = ",";
  }
  oss << "]";
  return casacore::String(oss);
}

inline casacore::String selectOnSourceAutoCorr(casacore::String const &msName) {
  casacore::MeasurementSet const ms(msName);
  debuglog<< "selectOnSource" << debugpost;
  casa::String taqlForState(
      "SELECT FLAG_ROW FROM $1 WHERE UPPER(OBS_MODE) ~ m/^OBSERVE_TARGET#ON_SOURCE/");
  casa::Table stateSel = casacore::tableCommand(taqlForState, ms.state());
  casa::Vector<casa::uInt> stateIdList = stateSel.rowNumbers();
  debuglog<< "stateIdList = " << stateIdList << debugpost;
  std::ostringstream oss;
  oss << "SELECT FROM \"" << msName
      << "\" WHERE ANTENNA1 == ANTENNA2 && STATE_ID IN "
      << toString(stateIdList)
      << " ORDER BY FIELD_ID, ANTENNA1, FEED1, DATA_DESC_ID, TIME";
  return casacore::String(oss);
}

class DataColumnAccessor {
public:
  DataColumnAccessor(casacore::Table const &ms,
  casacore::String const colName = "DATA") :
      dataCol_(ms, colName) {
  }
  casacore::Matrix<casacore::Float> operator()(size_t irow) {
    return casacore::real(dataCol_(irow));
  }
  casacore::Cube<casacore::Float> getColumn() {
    return casacore::real(dataCol_.getColumn());
  }
private:
  DataColumnAccessor() {
  }
  casacore::ROArrayColumn<casacore::Complex> dataCol_;
};

class FloatDataColumnAccessor {
public:
  FloatDataColumnAccessor(casacore::Table const &ms) :
      dataCol_(ms, "FLOAT_DATA") {
  }
  casacore::Matrix<casacore::Float> operator()(size_t irow) {
    return dataCol_(irow);
  }
  casacore::Cube<casacore::Float> getColumn() {
    return dataCol_.getColumn();
  }
private:
  FloatDataColumnAccessor() {
  }
  casa::ROArrayColumn<casa::Float> dataCol_;
};

inline bool isEphemeris(casa::String const &name) {
  // Check if given name is included in MDirection types
  casa::Int nall, nextra;
  const casa::uInt *typ;
  auto *types = casa::MDirection::allMyTypes(nall, nextra, typ);
  auto start_extra = nall - nextra;
  auto capital_name = name;
  capital_name.upcase();

  for (auto i = start_extra; i < nall; ++i) {
    if (capital_name == types[i]) {
      return true;
    }
  }

  return false;
}

inline void updateWeight(casa::NewCalTable &ct) {
  casa::CTMainColumns ctmc(ct);

  // simply copy FPARAM
  for (size_t irow = 0; irow < ct.nrow(); ++irow) {
    ctmc.weight().put(irow, ctmc.fparam()(irow));
  }
}}

namespace casa {
SDDoubleCircleGainCal::SDDoubleCircleGainCal(VisSet &vs) :
    VisCal(vs),             // virtual base
    VisMueller(vs),         // virtual base
    GJones(vs) {
}

SDDoubleCircleGainCal::~SDDoubleCircleGainCal() {
}

void SDDoubleCircleGainCal::selfGatherAndSolve(VisSet& vs, VisEquation& ve) {
  SDDoubleCircleGainCalImpl sdcalib;
  debuglog<< "SDDoubleCircleGainCal::selfGatherAndSolve()" << debugpost;

  // TODO: implement pre-application of single dish caltables

  debuglog<< "nspw = " << nSpw() << debugpost;
  fillNChanParList(msName(), nChanParList());
  debuglog<< "nChanParList=" << nChanParList() << debugpost;

  // Create a new caltable to fill up
  createMemCalTable();

  // Setup shape of solveAllRPar
  nElem() = 1;
  initSolvePar();

  // Pick up OFF spectra using STATE_ID
  debuglog<< "configure data selection for specific calibration mode" << debugpost;
  String taql = selectOnSourceAutoCorr(msName());
  debuglog<< "taql = \"" << taql << "\"" << debugpost;
  MeasurementSet msSel(tableCommand(taql, vs.iter().ms()));
  debuglog<< "msSel.nrow()=" << msSel.nrow() << debugpost;
  if (msSel.nrow() == 0) {
    throw AipsError("No reference integration in the data.");
  }
  String dataColName =
      (msSel.tableDesc().isColumn("FLOAT_DATA")) ? "FLOAT_DATA" : "DATA";
  debuglog<< "dataColName = " << dataColName << debugpost;

  if (msSel.tableDesc().isColumn("FLOAT_DATA")) {
    executeDoubleCircleGainCal<FloatDataColumnAccessor>(msSel);
  } else {
    executeDoubleCircleGainCal<DataColumnAccessor>(msSel);
  }

  //assignCTScanField(*ct_, msName());

  // update weight
  updateWeight(*ct_);

  // store caltable
  storeNCT();
}

template<class Accessor>
void SDDoubleCircleGainCal::executeDoubleCircleGainCal(
    MeasurementSet const &ms) {
  // setup worker class
  SDDoubleCircleGainCalImpl worker;
  // TODO: options for worker class should propagate here
  Double radius = 0.0;
  Bool doSmoothing = False;
  Int smoothingSize = 0;
  worker.setCentralRegion(radius);
  if (doSmoothing) {
    worker.setSmoothing(smoothingSize);
  } else {
    worker.unsetSmoothing();
  }

  ROArrayColumn<Double> uvwColumn(ms, "UVW");
  Matrix<Double> uvw = uvwColumn.getColumn();
  debuglog<< "uvw.shape " << uvw.shape() << debugpost;

  // make a map between SOURCE_ID and source NAME
  auto const &sourceTable = ms.source();
  ROScalarColumn<Int> idCol(sourceTable,
      sourceTable.columnName(MSSource::MSSourceEnums::SOURCE_ID));
  ROScalarColumn<String> nameCol(sourceTable,
      sourceTable.columnName(MSSource::MSSourceEnums::NAME));
  std::map<Int, String> sourceMap;
  for (uInt irow = 0; irow < sourceTable.nrow(); ++irow) {
    auto sourceId = idCol(irow);
    if (sourceMap.find(sourceId) == sourceMap.end()) {
      sourceMap[sourceId] = nameCol(irow);
    }
  }

  // make a map between FIELD_ID and SOURCE_ID
  auto const &fieldTable = ms.field();
  idCol.attach(fieldTable,
      fieldTable.columnName(MSField::MSFieldEnums::SOURCE_ID));
  std::map<Int, Int> fieldMap;
  for (uInt irow = 0; irow < fieldTable.nrow(); ++irow) {
    auto sourceId = idCol(irow);
    fieldMap[static_cast<Int>(irow)] = sourceId;
  }

  // access to subtable columns
  ROScalarQuantColumn<Double> antennaDiameterColumn(ms.antenna(),
      "DISH_DIAMETER");
  ROArrayQuantColumn<Double> observingFrequencyColumn(ms.spectralWindow(),
      "CHAN_FREQ");

  // traverse MS
  Int cols[] = { MS::FIELD_ID, MS::ANTENNA1, MS::FEED1, MS::DATA_DESC_ID };
  Int *colsp = cols;
  Block<Int> sortCols(4, colsp, False);
  MSIter msIter(ms, sortCols, 0.0, False, False);
  for (msIter.origin(); msIter.more(); msIter++) {
    MeasurementSet const currentMS = msIter.table();

    uInt nrow = currentMS.nrow();
    debuglog<< "nrow = " << nrow << debugpost;
    if (nrow == 0) {
      debuglog<< "Skip" << debugpost;
      continue;
    }
    Int ispw = msIter.spectralWindowId();
    if (nChanParList()[ispw] == 4) {
      // Skip WVR
      debuglog<< "Skip " << ispw
      << "(nchan " << nChanParList()[ispw] << ")"
      << debugpost;
      continue;
    }
    debuglog<< "Process " << ispw
    << "(nchan " << nChanParList()[ispw] << ")"
    << debugpost;

    Int ifield = msIter.fieldId();
    ROScalarColumn<Int> antennaCol(currentMS, "ANTENNA1");
    //currAnt_ = antennaCol(0);
    Int iantenna = antennaCol(0);
    ROScalarColumn<Int> feedCol(currentMS, "FEED1");
    debuglog<< "FIELD_ID " << msIter.fieldId()
    << " ANTENNA1 " << iantenna  //currAnt_
    << " FEED1 " << feedCol(0)
    << " DATA_DESC_ID " << msIter.dataDescriptionId()
    << debugpost;

    // setup PointingDirectionCalculator
    PointingDirectionCalculator pcalc(currentMS);
    pcalc.setDirectionColumn("DIRECTION");
    pcalc.setFrame("J2000");
    pcalc.setDirectionListMatrixShape(PointingDirectionCalculator::ROW_MAJOR);
    debuglog<< "SOURCE_ID " << fieldMap[ifield] << " SOURCE_NAME " << sourceMap[fieldMap[ifield]] << debugpost;
    if (::isEphemeris(sourceMap[fieldMap[ifield]])) {
      pcalc.setMovingSource(sourceMap[fieldMap[ifield]]);
    } else {
      pcalc.unsetMovingSource();
    }
    Matrix<Double> direction = pcalc.getDirection();
//    debuglog<< "direction = " << direction << debugpost;
    Double const *direction_p = direction.data();
    for (size_t i = 0; i < 10; ++i) {
      debuglog<< "direction[" << i << "]=" << direction_p[i] << debugpost;
    }

    ROScalarColumn<Double> timeCol(currentMS, "TIME");
    Vector<Double> time = timeCol.getColumn();
//    debuglog<< "time = " << time << debugpost;
//  ROScalarColumn<Double> exposureCol(current, "EXPOSURE");
//  ROScalarColumn<Double> intervalCol(current, "INTERVAL");
    Accessor dataCol(currentMS);
    Cube<Float> data = dataCol.getColumn();
//    debuglog<< "data = " << data << debugpost;

    Vector<Double> gainTime;
    Cube<Float> gain;
    Quantity antennaDiameterQuant = antennaDiameterColumn(iantenna);
    worker.setAntennaDiameter(antennaDiameterQuant.getValue("m"));
    debuglog<< "antenna diameter = " << worker.getAntennaDiameter() << "m" << debugpost;
    Vector<Quantity> observingFrequencyQuant = observingFrequencyColumn(ispw);
    Double meanFrequency = 0.0;
    auto numChan = observingFrequencyQuant.nelements();
    debuglog<< "numChan = " << numChan << debugpost;
    assert(numChan > 0);
    if (numChan % 2 == 0) {
      meanFrequency = (observingFrequencyQuant[numChan / 2 - 1].getValue("Hz")
          + observingFrequencyQuant[numChan / 2].getValue("Hz")) / 2.0;
    } else {
      meanFrequency = observingFrequencyQuant[numChan / 2].getValue("Hz");
    }
    //debuglog << "mean frequency " << meanFrequency.getValue() << " [" << meanFrequency.getFullUnit() << "]" << debugpost;
    debuglog<< "mean frequency " << meanFrequency << debugpost;
    worker.setObservingFrequency(meanFrequency);
    debuglog<< "observing frequency = " << worker.getObservingFrequency() / 1e9 << "GHz" << debugpost;
    Double primaryBeamSize = worker.getPrimaryBeamSize();
    debuglog<< "primary beam size = " << primaryBeamSize << " arcsec" << debugpost;
    worker.calibrate(data, time, direction, gainTime, gain);
    //debuglog<< "gain_time = " << gain_time << debugpost;
    //debuglog<<"gain = " << gain << debugpost;
    size_t numGain = gainTime.nelements();
    debuglog<< "number of gain " << numGain << debugpost;

    ROArrayColumn<Bool> flagCol(currentMS, "FLAG");
    Cube<Bool> flag = flagCol.getColumn();

    currSpw() = ispw;
    currField() = ifield;

    solveAllParErr() = 0.1; // TODO
    solveAllParSNR() = 1.0; // TODO

    size_t numCorr = gain.shape()[0];
    Slice corrSlice(0, numCorr);
    Slice chanSlice(0, numChan);
    Cube<Bool> allParOK(numCorr, numChan, 1, True);
    for (size_t i = 0; i < numGain; ++i) {
      refTime() = gainTime[i];
      solveAllRPar() = gain(corrSlice, chanSlice, Slice(i, 1));
      solveAllParOK() = allParOK;

      keepNCT();
    }

  }
}
}