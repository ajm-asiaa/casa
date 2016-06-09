/*
 * SDDoubleCircleGainCal.h
 *
 *  Created on: Jun 3, 2016
 *      Author: nakazato
 */

#ifndef SYNTHESIS_MEASUREMENTCOMPONENTS_SDDOUBLECIRCLEGAINCAL_H_
#define SYNTHESIS_MEASUREMENTCOMPONENTS_SDDOUBLECIRCLEGAINCAL_H_

#include <synthesis/MeasurementComponents/StandardVisCal.h>
#include <casacore/casa/BasicSL/String.h>

namespace casa {

class VisSet;
class VisEquation;

class SDDoubleCircleGainCal: public GJones {
public:
  SDDoubleCircleGainCal(VisSet& vs);
  virtual ~SDDoubleCircleGainCal();

  // Return type name as string (ditto)
  virtual casacore::String typeName() {
    return "SDGAIN_OTFD";
  }
  virtual casacore::String longTypeName() {
    return "SDGAIN_OTFD (Single Dish gain calibration for double circle fast scan";
  }

  // Return the parameter type
  // so far single dish calibration is real
  virtual VisCalEnum::VCParType parType() { return VisCalEnum::REAL; }

  // Do not use generic data gathering mechanism for solve
  virtual Bool useGenericGatherForSolve() {
    return False;
  }

  // Self- gather and/or solve prototypes
  //  (triggered by useGenericGatherForSolve=F or useGenericSolveOne=F)
  virtual void selfGatherAndSolve(VisSet& vs, VisEquation& ve);

private:
  template<class Accessor>
  void executeDoubleCircleGainCal(casacore::MeasurementSet const &ms);
};

} // namespace casa END

#endif /* SYNTHESIS_MEASUREMENTCOMPONENTS_SDDOUBLECIRCLEGAINCAL_H_ */