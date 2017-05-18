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

#include "AitkenPredictor.h"
#include "NonlinearSystem.h"
//#include "FEProblem.h"
//#include "FEProblemBase.h"
#include "Coupleable.h"
#include "MooseVariable.h"

template<>
InputParameters validParams<AitkenPredictor>()
{
  InputParameters params = validParams<Predictor>();
  //params.addParam<int>("order", 2, "The maximum reachable order of the Adams-Bashforth Predictor");
  //TODO: give a parameter that let's users specify more than two steps between acceleration
  //TODO: change that into the theta criterion
  params.addRequiredCoupledVar("transferred_value","The AuxVar transferred from the other solver to be relaxed");
  return params;
}

AitkenPredictor::AitkenPredictor(const InputParameters & parameters) :
    Predictor(parameters),
   // CoupleableMooseVariableDependencyIntermediateInterface(this, false),
    //_order(getParam<int>("order")),
    /*_current_old_solution(_nl.addVector("AB2_current_old_solution", true, GHOSTED)),
    _older_solution(_nl.addVector("AB2_older_solution", true, GHOSTED)),
    _oldest_solution(_nl.addVector("AB2_rejected_solution", true, GHOSTED)),
    _tmp_previous_solution(_nl.addVector("tmp_previous_solution", true, GHOSTED)), */
    _tmp_vec1(_nl.addVector("tmp_vec1", true, GHOSTED)),
    _tmp_vec2(_nl.addVector("tmp_vec2", true, GHOSTED)),
    //_t_step_old(declareRestartableData<int>("t_step_old", 0)),
    //_dt_older(declareRestartableData<Real>("dt_older", 0)),
    //_dtstorage(declareRestartableData<Real>("dtstorage", 0)),
    _counter(0)
   /* _transf_var(coupledValue("transferred_value")),
    _transf_var_old(coupledValueOld("transferred_value")),
    _transf_var_older(coupledValueOlder("transferred_value")) *///var
{
}

AitkenPredictor::~AitkenPredictor()
{
}
/*
void
AitkenPredictor::historyControl()
{
  // if the time step number hasn't changed then do nothing
  if (_t_step == _t_step_old)
    return;

  // Otherwise move back the previous old solution and copy the current old solution,
  // This will probably not work with DT2, but I don't need to get it to work with dt2.
  _t_step_old = _t_step;

  */ /*_older_solution.localize(_oldest_solution);
  // Set older solution to hold the previous old solution
  _current_old_solution.localize(_older_solution);
  // Set current old solution to hold what it says.
  (_nl.solutionOld()).localize(_current_old_solution);*/ /*
  // or can we just use the variables that already exist?
  _solution_old.localize(_solution_older);
  _solution.localize(_solution_old);
  //Same thing for dt
  _dt_older = _dtstorage;
  _dtstorage = _dt_old;
}*/

// You may want/need a reset_Aitken function after you accelerate

void
AitkenPredictor::apply(NumericVector<Number> & sln)
{
  // At the moment, I don't believe there is a function in Predictor
  // that gets called on time step begin.
  // That means that history control must go here.
  //   historyControl();  //Jaron doesn't think we need this
  // AB2 can only be applied if there are enough old solutions
  // AB1 could potentially be used for the time step prior?
  // It would be possible to do VSVO Adams, Kevin has the info
  // Doing so requires a time stack of some sort....

  if (_counter<2)
  {
    //history_control
    _counter++; // verify that this is incremented only at the right time
    return;
  }
  /*if (_dt == 0 || _dt_old == 0 || _dt_older == 0 || _t_step < 2)
    return; */

  _console << "Applying Aitken Acceleration" << std::endl;
  _counter = 0;  


  //Real _numerator = (_current_old_solution -_older_solution)*(_older_solution -_oldest_solution);
  //Real _denominator = (_older_solution-_oldest_solution)*(_current_old_solution -2*_older_solution +_oldest_solution);
  //   (are we in parallel decomposition? are we repeating on each mpi?)
  Real _numerator =0;
  Real _denominator =0;
  sln.localize(_tmp_vec1);
  _solution_old.localize(_tmp_vec2);
  _tmp_vec1.add(-1.0, _solution_old);// += -1.0 * _solution_old; //
  _tmp_vec2.add(-1.0, _solution_older);
  _numerator = _tmp_vec1.dot(_tmp_vec2 );
  _solution_old.localize(_tmp_vec1);
  _tmp_vec1.add(-1.0, _solution_older);
  sln.localize(_tmp_vec2);
  _tmp_vec2.add(_solution_older);
  _tmp_vec2.add(-2.0,_solution_old);
  _denominator = _tmp_vec1.dot(_tmp_vec2 );
/*  for(unsigned int _it = 0; _it < _transf_var.size(); _it++)
  {
    _numerator += (_transf_var[_it] - _transf_var_old[_it]) *
      (_transf_var_old[_it] - _transf_var_older[_it]);
    _denominator += (_transf_var_old[_it] - _transf_var_older[_it]) *
      (_transf_var[_it] - 2.0 * _transf_var_old[_it] + _transf_var_older[_it]);
  } *///var
 
  if(_denominator!=0)
 {
  Real _s = _numerator / _denominator;

  // See below for this awkward implementation
  //_solution_predictor = _current_old_solution -_s *(_current_old_solution-_older_solution);
  //
  // localize current solution to working vec

  sln.localize(_solution_predictor);
  _solution_predictor -= _solution_old;
  sln.add(-_s, _solution_predictor); //_solution_predictor.localize(sln);

  /*
//_transf_var = _transf_var - _s * (_transf_var - _transf_var_old);
  for(unsigned int _it = 0; _it < _transf_var.size(); _it++)
  {
//need to dereference or something
    _transf_var[_it] = _transf_var[_it] - _s * (_transf_var[_it] - _transf_var_old[_it]);
  } *///var
 }
}

bool
AitkenPredictor::shouldApply()
{
//  if (!Predictor::shouldApply())
//    return false;
 // if(_t_step < 2)
  //  return false;
  _console << "shouldApply=true\n";
  return true;
}

