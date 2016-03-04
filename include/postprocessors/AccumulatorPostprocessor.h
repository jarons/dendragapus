/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/
#ifndef ACCUMULATORPOSTPROCESSOR_H
#define ACCUMULATORPOSTPROCESSOR_H

#include "GeneralPostprocessor.h"

class AccumulatorPostprocessor;

template<>
InputParameters validParams<AccumulatorPostprocessor>();

/**
 * Accumulates the value of another postprocessor over time
 *
 * result += postprocessor(current_time) 
 */
class AccumulatorPostprocessor : public GeneralPostprocessor
{
public:
  AccumulatorPostprocessor(const InputParameters & parameters);
  virtual ~AccumulatorPostprocessor();

  virtual void initialize();
  virtual void execute();
  virtual PostprocessorValue getValue();
  virtual void threadJoin(const UserObject & uo);

protected:
  const PostprocessorValue & _value;
  //const PostprocessorValue & _value2;
  Real _my_value;
};


#endif /* ACCUMULATORPOSTPROCESSOR_H */
