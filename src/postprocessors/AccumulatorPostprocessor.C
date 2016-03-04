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
#include "AccumulatorPostprocessor.h"

template<>
InputParameters validParams<AccumulatorPostprocessor>()
{
  InputParameters params = validParams<GeneralPostprocessor>();
  params.addRequiredParam<PostprocessorName>("value", "Coupled postprocessor to accumulate across timesteps");
  params.addParam<Real>("Initial_value",0.0,"If you don't want the accumulation to start at zero");
  //params.addRequiredParam<PostprocessorName>("value2", "Second value");

  return params;
}

AccumulatorPostprocessor::AccumulatorPostprocessor(const InputParameters & parameters) :
    GeneralPostprocessor(parameters),
    _value(getPostprocessorValue("value")),
    _my_value(getParam<Real>("Initial_value"))
    //_value2(getPostprocessorValue("value2"))
    // for time integrator _dt(...
{
}

AccumulatorPostprocessor::~AccumulatorPostprocessor()
{
}

void
AccumulatorPostprocessor::initialize()
{
}

void
AccumulatorPostprocessor::execute()
{
  _my_value += _value;
}

PostprocessorValue
AccumulatorPostprocessor::getValue()
{
  return _my_value;
}

void
AccumulatorPostprocessor::threadJoin(const UserObject & /*uo*/)
{
  // nothing to do here, general PPS do not run threaded
}
