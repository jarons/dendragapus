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

#include "Transient.h"
#include "InterruptibleTransient.h"

//Moose includes
#include "Factory.h"
#include "SubProblem.h"
#include "TimePeriod.h"
#include "TimeStepper.h"
#include "MooseApp.h"
#include "Conversion.h"
//libMesh includes
#include "libmesh/implicit_system.h"
#include "libmesh/nonlinear_implicit_system.h"
#include "libmesh/transient_system.h"
#include "libmesh/numeric_vector.h"

// C++ Includes
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

template<>
InputParameters validParams<InterruptibleTransient>()
{
  InputParameters params = validParams<Transient>(); 
  return params;
}

InterruptibleTransient::InterruptibleTransient(const InputParameters & parameters) :
    Transient( parameters) 
{ /*
  _problem.getNonlinearSystem().setDecomposition(_splitting);
  _t_step = 0;
  _dt = 0;
  _next_interval_output_time = 0.0;

  // Either a start_time has been forced on us, or we want to tell the App about what our start time is (in case anyone else is interested.
  if (_app.hasStartTime())
    _start_time = _app.getStartTime();
  else
    _app.setStartTime(_start_time);

  _time = _time_old = _start_time;
  _problem.transient(true);

  if (parameters.isParamValid("predictor_scale"))
  {
    mooseWarning("Parameter 'predictor_scale' is deprecated, migrate your input file to use Predictor sub-block.");

    Real predscale = getParam<Real>("predictor_scale");
    if (predscale >= 0.0 && predscale <= 1.0)
    {
      InputParameters params = _app.getFactory().getValidParams("SimplePredictor");
      params.set<Real>("scale") = predscale;
      _problem.addPredictor("SimplePredictor", "predictor", params);
    }

    else
      mooseError("Input value for predictor_scale = "<< predscale << ", outside of permissible range (0 to 1)");

  }

  if (!_restart_file_base.empty())
    _problem.setRestartFile(_restart_file_base);

  setupTimeIntegrator();

  if (_app.halfTransient()) // Cut timesteps and end_time in half...
  {
    _end_time /= 2.0;
    _num_steps /= 2.0;

    if (_num_steps == 0) // Always do one step in the first half
      _num_steps = 1;
  } */
}

InterruptibleTransient::~InterruptibleTransient()
{
}

void
InterruptibleTransient::init()
{
  if (!_time_stepper.get())
  {
    InputParameters pars = _app.getFactory().getValidParams("ConstantDT");
    pars.set<FEProblemBase *>("_fe_problem_base") = &_problem;
    pars.set<Transient *>("_executioner") = this;

    /**
     * We have a default "dt" set in the Transient parameters but it's possible for users to set other
     * parameters explicitly that could provide a better calculated "dt". Rather than provide difficult
     * to understand behavior using the default "dt" in this case, we'll calculate "dt" properly.
     */
    if (!_pars.isParamSetByAddParam("end_time") && !_pars.isParamSetByAddParam("num_steps") && _pars.isParamSetByAddParam("dt"))
      pars.set<Real>("dt") = (getParam<Real>("end_time") - getParam<Real>("start_time")) / static_cast<Real>(getParam<unsigned int>("num_steps"));
    else
      pars.set<Real>("dt") = getParam<Real>("dt");

    pars.set<bool>("reset_dt") = getParam<bool>("reset_dt");
    _time_stepper = _app.getFactory().create<TimeStepper>("ConstantDT", "TimeStepper", pars);
  }

  _problem.initialSetup();
  _time_stepper->init();

  if (_app.isRestarting())
    _time_old = _time;

  _problem.outputStep(EXEC_INITIAL);

  // If this is the first step
  if (_t_step == 0)
    _t_step = 1;

  if (_t_step > 1) //Recover case
    _dt_old = _dt;

  else
  {
    computeDT();
//  _dt = computeConstrainedDT();
    _dt = getDT();
  }


}

void
InterruptibleTransient::incrementStepOrReject()
{
  if (_last_solve_converged)
  {
#ifdef LIBMESH_ENABLE_AMR
    if (_problem.adaptivity().isOn())
      _problem.adaptMesh();
#endif

    _time_old = _time; // = _time_old + _dt;
    _t_step++;
    // This was called, your problem is elsewhere
    _console << "Regular incrementStep" << std::endl;
    _problem.advanceState();
  }
  else
  {
    _time_stepper->rejectStep();
    _time = _time_old;
  }

  _first = false;

}

void
InterruptibleTransient::re_incrementStepOrReject()
{
  if (_last_solve_converged)
  {
#ifdef LIBMESH_ENABLE_AMR
    if (_problem.adaptivity().isOn())
      _problem.adaptMesh();
#endif

    // original: "_time_old = _time; // = _time_old + _dt;"
    //_time_old = _time; // = _time_old + _dt;
    //_t_step++;      //This works
    
    //_console << "Kilroy was here. _time = " << _time << std::endl;
    //_problem.advanceState();
  }
  else
  {
    _time_stepper->rejectStep();
    _time = _time_old;
  }

  _first = false;

}

void
InterruptibleTransient::takeStep(Real input_dt)
{
  _picard_it = 0;
  while (_picard_it<_picard_max_its && _picard_converged == false)
  {
    if (_picard_max_its > 1)
      _console << "Beginning Picard Iteration " << _picard_it << "\n" << std::endl;

    solveStep(input_dt);
    ++_picard_it;
  }
}

void
InterruptibleTransient::re_takeStep(Real input_dt)
{
  _picard_it = 0;
  while (_picard_it<_picard_max_its && _picard_converged == false)
  {
    if (_picard_max_its > 1)
      _console << "Beginning Picard Iteration " << _picard_it << "\n" << std::endl;

    re_solveStep(input_dt);
    ++_picard_it;
  }
}

void
InterruptibleTransient::solveStep(Real input_dt)
{
  _dt_old = _dt;

  if (input_dt == -1.0)
    _dt = computeConstrainedDT();
  else
    _dt = input_dt;

  Real current_dt = _dt;

  _problem.onTimestepBegin();

  // Increment time
  _time = _time_old + _dt;

  _problem.execTransfers(EXEC_TIMESTEP_BEGIN);
  _problem.execMultiApps(EXEC_TIMESTEP_BEGIN, _picard_max_its == 1);

  preSolve();
  _time_stepper->preSolve();

  _problem.timestepSetup();
/*
  // Compute Pre-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::PRE_AUX);

  // Compute TimestepBegin AuxKernels
  _problem.computeAuxiliaryKernels(EXEC_TIMESTEP_BEGIN);

  // Compute Post-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::POST_AUX);

  // Perform output for timestep begin
  _problem.outputStep(EXEC_TIMESTEP_BEGIN); */
  _problem.execute(EXEC_TIMESTEP_BEGIN);

  if (_picard_max_its > 1)
  {
    Real current_norm = _problem.computeResidualL2Norm();
    if (_picard_it == 0) // First Picard iteration - need to save off the initial nonlinear residual
    {
      _picard_initial_norm = current_norm;
      _console << "Initial Picard Norm: " << _picard_initial_norm << '\n';
    }
    else
      _console << "Current Picard Norm: " << current_norm << '\n';

    Real relative_drop = current_norm / _picard_initial_norm;

    if (current_norm < _picard_abs_tol || relative_drop < _picard_rel_tol)
    {
      _console << "Picard converged!" << std::endl;

      _picard_converged = true;
      _time_stepper->acceptStep();
      return;
    }
  }
  

  _time_stepper->step();

  // We know whether or not the nonlinear solver thinks it converged, but we need to see if the executioner concurs
  // Need to add a check to see when the sub-app is actually converged. I would like to avoid transferring initial and final residual postprocs, but probably need to.
  if (lastSolveConverged())
  {
    _console << COLOR_GREEN << " Solve Converged!" << COLOR_DEFAULT << std::endl;

    if (_picard_max_its <= 1 )
      _time_stepper->acceptStep();

    _solution_change_norm = _problem.relativeSolutionDifferenceNorm();
/*
    _problem.computeUserObjects(EXEC_TIMESTEP_END, UserObjectWarehouse::PRE_AUX);
#if 0
    // User definable callback
    if (_estimate_error)
      estimateTimeError();
#endif

    _problem.onTimestepEnd();

    _problem.computeAuxiliaryKernels(EXEC_TIMESTEP_END);
    _problem.computeUserObjects(EXEC_TIMESTEP_END, UserObjectWarehouse::POST_AUX);
    _problem.execTransfers(EXEC_TIMESTEP_END);
    _problem.execMultiApps(EXEC_TIMESTEP_END, _picard_max_its == 1); */
      _problem.onTimestepEnd();
      _problem.execute(EXEC_TIMESTEP_END);

      _problem.execTransfers(EXEC_TIMESTEP_END);
      _multiapps_converged = _problem.execMultiApps(EXEC_TIMESTEP_END, _picard_max_its == 1);

      if (!_multiapps_converged)
        return;

  }
  else
  {
    _console << COLOR_RED << " Solve Did NOT Converge!" << COLOR_DEFAULT << std::endl;

    // Perform the output of the current, failed time step (this only occurs if desired)
    _problem.outputStep(EXEC_FAILED);
  }

  postSolve();
  _time_stepper->postSolve();
  _dt = current_dt; // _dt might be smaller than this at this point for multistep methods
  _time = _time_old;
}

void
InterruptibleTransient::re_solveStep(Real input_dt)
{
  _dt_old = _dt;

  if (input_dt == -1.0)
    _dt = computeConstrainedDT();
  else
    _dt = input_dt;

  Real current_dt = _dt;

  _problem.onTimestepBegin();

  // Increment time
  _time = _time_old + _dt;
  // Does the following work/help?
  //_time = _time_old;

  _problem.execTransfers(EXEC_TIMESTEP_BEGIN);
  _problem.execMultiApps(EXEC_TIMESTEP_BEGIN, _picard_max_its == 1);

  preSolve();
  _time_stepper->preSolve();

  _problem.timestepSetup();
/*
  // Compute Pre-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::PRE_AUX);

  // Compute TimestepBegin AuxKernels
  _problem.computeAuxiliaryKernels(EXEC_TIMESTEP_BEGIN);

  // Compute Post-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::POST_AUX);

  // Perform output for timestep begin
  _problem.outputStep(EXEC_TIMESTEP_BEGIN); */
  _problem.execute(EXEC_TIMESTEP_BEGIN);

  /*if (_picard_max_its > 1)
  {
    Real current_norm = _problem.computeResidualL2Norm();
    if (_picard_it == 0) // First Picard iteration - need to save off the initial nonlinear residual
    {
      _picard_initial_norm = current_norm;
      _console << "Initial Picard Norm: " << _picard_initial_norm << '\n';
    }
    else
      _console << "Current Picard Norm: " << current_norm << '\n';

    Real relative_drop = current_norm / _picard_initial_norm;

    if (current_norm < _picard_abs_tol || relative_drop < _picard_rel_tol)
    {
      _console << "Picard converged!" << std::endl;

      _picard_converged = true;
      _time_stepper->acceptStep();
      return;
    }
  } */
  // Do the same logic for a re-solve multiapp as for Picard
  // Because we want to acceptStep after doing time=time_old
  //if( _problem.is_re_solved() )
  /*if (lastSolveConverged())
  {
    _time_stepper->acceptStep();
    return; 
  } */
  //

  _time_stepper->step();

  // We know whether or not the nonlinear solver thinks it converged, but we need to see if the executioner concurs
  if (lastSolveConverged())
  {
    _console << COLOR_GREEN << " Solve Converged!" << COLOR_DEFAULT << std::endl;

    if (_picard_max_its <= 1 ) 
    { 
      _time_stepper->acceptStep();
    }

    _solution_change_norm = _problem.relativeSolutionDifferenceNorm();
/* Replace this stuff with the following
    _problem.computeUserObjects(EXEC_TIMESTEP_END, UserObjectWarehouse::PRE_AUX);
#if 0
    // User definable callback
    if (_estimate_error)
      estimateTimeError();
#endif

    _problem.onTimestepEnd();

    _problem.computeAuxiliaryKernels(EXEC_TIMESTEP_END);
    _problem.computeUserObjects(EXEC_TIMESTEP_END, UserObjectWarehouse::POST_AUX);
    _problem.execTransfers(EXEC_TIMESTEP_END);
    _problem.execMultiApps(EXEC_TIMESTEP_END, _picard_max_its == 1); */
    
      _problem.onTimestepEnd();
      _problem.execute(EXEC_TIMESTEP_END);

      _problem.execTransfers(EXEC_TIMESTEP_END);
      _multiapps_converged = _problem.execMultiApps(EXEC_TIMESTEP_END, _picard_max_its == 1);

      if (!_multiapps_converged)
        return;

  }
  else
  {
    _console << COLOR_RED << " Solve Did NOT Converge!" << COLOR_DEFAULT << std::endl;

    // Perform the output of the current, failed time step (this only occurs if desired)
    _problem.outputStep(EXEC_FAILED);
  }

  postSolve();
  _time_stepper->postSolve();
  _dt = current_dt; // _dt might be smaller than this at this point for multistep methods
  _time = _time_old;
}

void
InterruptibleTransient::endStep(Real input_time)
{
  if (input_time == -1.0)
    _time = _time_old + _dt;
  else
    _time = input_time;

  _picard_converged=false;

  _last_solve_converged = lastSolveConverged();

  if (_last_solve_converged)
  {
    // Compute the Error Indicators and Markers
    _problem.computeIndicatorsAndMarkers();

    // Perform the output of the current time step
    _problem.outputStep(EXEC_TIMESTEP_END);

    // Output MultiApps if we were doing Picard iterations
    if (_picard_max_its > 1)
    {
      _problem.advanceMultiApps(EXEC_TIMESTEP_BEGIN);
      _problem.advanceMultiApps(EXEC_TIMESTEP_END);
    }

		//Jaron was here
    _problem.advanceMultiApps(EXEC_NONLINEAR);

    //output
    if (_time_interval && (_time + _timestep_tolerance >= _next_interval_output_time))
      _next_interval_output_time += _time_interval_output_interval;
   }
}

