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
#include "ResidualBalanceTransient.h"

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
#include <algorithm> // min(a,b)

template<>
InputParameters validParams<ResidualBalanceTransient>()
{
  InputParameters params = validParams<Transient>(); 
  params.addParam<Real>("tol_mult",1,"(Other residual)*this=(new abs_tol)"); 
  params.addRequiredParam<PostprocessorName>("InitialResidual", "The name of the postprocessor you are trying to get.");
  return params;
}

ResidualBalanceTransient::ResidualBalanceTransient(const std::string & name, InputParameters parameters) :
    Transient(name, parameters), 
    _new_tol_mult(getParam<Real>("tol_mult")),
    _new_tol(getPostprocessorValue("InitialResidual"))  
{
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
  }
}

ResidualBalanceTransient::~ResidualBalanceTransient()
{
}

void
ResidualBalanceTransient::solveStep(Real input_dt)
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

  // Compute Pre-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::PRE_AUX);

  // Compute TimestepBegin AuxKernels
  _problem.computeAuxiliaryKernels(EXEC_TIMESTEP_BEGIN);

  // Compute Post-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::POST_AUX);

  // Perform output for timestep begin
  _problem.outputStep(EXEC_TIMESTEP_BEGIN);

  //Real current_norm; //moved this line to .h file
  my_current_norm = _problem.computeResidualL2Norm(); // the norm from this app only
  //bool _adjust_initial_norm;
  his_initial_norm = getPostprocessorValue("InitialResidual");
  if (_picard_max_its > 1)
  {
    current_norm = my_current_norm + his_initial_norm;
    // this is imprecise: it will be (other_initial_norm + current_norm) instead of 
    //   (other_current_norm + current_norm). But that shouldn't be too bad, because it's only
    //   taken one step since then. To improve, get access to the ending residual also (another PP).

    if (_picard_it == 0) // First Picard iteration - need to save off the initial nonlinear residual
    {
      _picard_initial_norm = current_norm;
      _console << "Initial Picard Norm: " << _picard_initial_norm << '\n';
      if (his_initial_norm == 0)
        _adjust_initial_norm = true;
    }
    else
      _console << "Current Picard Norm: " << current_norm << '\n';

    if (_picard_it == 1 && _adjust_initial_norm == true)
    {
      _picard_initial_norm = _picard_initial_norm + his_initial_norm;
      _console << "Adjusted Initial Picard Norm: " << _picard_initial_norm << '\n';
    }

    Real relative_drop = current_norm / _picard_initial_norm;

    if (current_norm < _picard_abs_tol || relative_drop < _picard_rel_tol)
    {
      _console << "Picard converged!" << std::endl;

      _picard_converged = true;
      _time_stepper->acceptStep();
      return;
    }
  }
  
  // For multiple sub-apps you'll need an aggregate PP to collect residuals in
  //    or some way to keep the number of PPs low.
  if (his_initial_norm ==0)
  { his_initial_norm = my_current_norm; } // this statement is nearly meaningless, just ensure it won't revert us back to Method A
  _new_tol = std::min(his_initial_norm*_new_tol_mult, 0.95*my_current_norm);
  // you may want 0.95 to be a parameter for the user to (not) change
  _console << "New Abs_Tol = " << _new_tol << std::endl;
  _problem.es().parameters.set<Real> ("nonlinear solver absolute residual tolerance") = _new_tol;


  _time_stepper->step();

  // We know whether or not the nonlinear solver thinks it converged, but we need to see if the executioner concurs
  if (lastSolveConverged())
  {
    _console << COLOR_GREEN << " Solve Converged!" << COLOR_DEFAULT << std::endl;

    if (_picard_max_its <= 1 )
      _time_stepper->acceptStep();

    _solution_change_norm = _problem.solutionChangeNorm();

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
    _problem.execMultiApps(EXEC_TIMESTEP_END, _picard_max_its == 1);
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
ResidualBalanceTransient::endStep(Real input_time)
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

		//Jaron was here. //Is this needed?
    _problem.advanceMultiApps(EXEC_NONLINEAR);

    //output
    if (_time_interval && (_time + _timestep_tolerance >= _next_interval_output_time))
      _next_interval_output_time += _time_interval_output_interval;
   }
}

