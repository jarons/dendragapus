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

InitialResidual::InitialResidual(const std::string & name, InputParameters parameters) :
    GeneralPostprocessor(name, parameters),
  _root_id(0), // for MPI
  _value(0)
{}

void 
InitialResidual::execute()
{
  _value=_fe_problem.getNonlinearSystem()._initial_residual;
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

// I think this block is not needed. Taken from PointValuePP
/*
 // Compute the value at the point
 if (_root_id == processor_id())
 {
 const Elem * elem = _mesh.elem(_elem_id);
 std::set<MooseVariable *> var_list;
 var_list.insert(&_var);

 _fe_problem.setActiveElementalMooseVariables(var_list, _tid);
 _subproblem.reinitElemPhys(elem, _point_vec, 0);
 mooseAssert(_u.size() == 1, "No values in u!");
 _value = _u[0];
 }
*/

 // Make sure all processors have the correct computed values
 _communicator.broadcast(_value, _root_id);
}
