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

#include "InitialResidual.h"

#include "FEProblem.h"
#include "SubProblem.h"

template<>
InputParameters validParams<InitialResidual>()
{
  InputParameters params = validParams<GeneralPostprocessor>();
  return params;
}

InitialResidual::InitialResidual(const InputParameters & parameters) :
    GeneralPostprocessor(parameters),
  _root_id(0),
  _value(0)
{}

void 
InitialResidual::execute()
{
  //_value=_fe_problem.getNonlinearSystem()._initial_residual;
  _value=_fe_problem.getNonlinearSystem()._initial_residual_before_preset_bcs;
}

Real
InitialResidual::getValue()
{
  //return _fe_problem.getNonlinearSystem()._initial_residual;  
  return _value;

}

void
InitialResidual::finalize()
{
 // Gather a consist id for broadcasting the computed value
 gatherMin(_root_id);

 // Make sure all processors have the correct computed values
 _communicator.broadcast(_value, _root_id);
}
