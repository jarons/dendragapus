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

#include "InitialResidual.h"

template<>
InputParameters validParams<ResidualBalanceTransient>()
{
  InputParameters params = validParams<Transient>(); 
  params.addParam<Real>("tol_mult",1,"(Other residual)*this=(new_abs_tol)"); 
  params.addRequiredParam<PostprocessorName>("InitialResidual", "The name of the InitialResidual postprocessor you are trying to get.");
  params.addParam<PostprocessorName>("FinalResidual",0.0, "The name of the Residual postprocessor you are trying to get.");
  params.addParam<Real>("nl_abs_tol",1e-50,"Set this to the same as nl_abs_tol"); //this overrides other nl_abs_tol
  
  params.addParamNamesToGroup("nl_abs_tol", "Solver"); // put it back into the Solver tab
  return params;
}

ResidualBalanceTransient::ResidualBalanceTransient(const InputParameters & parameters) :
    Transient(parameters), 
    _new_tol_mult(getParam<Real>("tol_mult")),
    _new_tol(getPostprocessorValue("InitialResidual")),
    his_final_norm(getPostprocessorValue("FinalResidual")),
    _min_abs_tol(getParam<Real>("nl_abs_tol"))
    //his_initial_norm_old(-1.0)
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

  // You could evaluate/store his_initial_norm_old  here
  
  _problem.execTransfers(EXEC_TIMESTEP_BEGIN);
  _multiapps_converged = _problem.execMultiApps(EXEC_TIMESTEP_BEGIN, _picard_max_its == 1);

  if (!_multiapps_converged)
    return;

  preSolve();
  _time_stepper->preSolve();

  _problem.timestepSetup();
 /*  These lines replaced by the following one
  // Compute Pre-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::PRE_AUX);

  // Compute TimestepBegin AuxKernels
  _problem.computeAuxiliaryKernels(EXEC_TIMESTEP_BEGIN);

  // Compute Post-Aux User Objects (Timestep begin)
  _problem.computeUserObjects(EXEC_TIMESTEP_BEGIN, UserObjectWarehouse::POST_AUX);

  // Perform output for timestep begin
  _problem.outputStep(EXEC_TIMESTEP_BEGIN);  */
  
  _problem.execute(EXEC_TIMESTEP_BEGIN);
  
  // Perform output for timestep begin
  _problem.outputStep(EXEC_TIMESTEP_BEGIN);

  // Update warehouse active objects
  _problem.updateActiveObjects();


  my_current_norm = _problem.computeResidualL2Norm(); // the norm from this app only
  his_initial_norm = getPostprocessorValue("InitialResidual");
    //for sub-app@timestep_begin: his_initial_norm should be zero at each timestep
  
  // if this is sub app and its the initial_residual wasn't updated 
      // or use his_initial_norm_old
  if (_picard_max_its==1 && his_initial_norm == getPostprocessorValueOld("InitialResidual"))
    his_initial_norm = 0;
  
  his_final_norm = getPostprocessorValue("FinalResidual");
    //TODO: throw a warning (not error) if no FinalResidual is selected in input file
  if (_picard_max_its > 1)
  {    
      // is there a way to get his_final_norm without a postprocessor transfer?
      //std::vector<MultiApp *> multi_apps = _problem._multi_apps(EXEC_TIMESTEP_END)[0].all();
          // unfortunately _multi_apps is protected
      //Real his_last_norm = multi_apps[i]._subproblem.finalNonlinearResidual();
      //_console << "Multi-app end residual" << his_final_norm << std::endl; // did it work?

    if (_picard_it == 0) // First Picard iteration - need to save off the initial nonlinear residual
    {
      current_norm = my_current_norm;  
      //his_initial_norm_old) //EXEC_TIMESTEP_END
      if (his_initial_norm == getPostprocessorValueOld("InitialResidual")) 
        his_initial_norm = 0; 
      // set his_initial_norm to 0 so that we can start a new timestep properly
      
      _picard_initial_norm = current_norm + his_initial_norm; 
      if (his_initial_norm == 0)
        _adjust_initial_norm = true;
      else
        _console << "Initial Picard Norm: " << _picard_initial_norm << '\n';
    }
    else
    {
      current_norm = my_current_norm + his_final_norm; 
      _console << "Current Picard Norm: " << current_norm << '\n';
    }

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

      // Accumulator Postprocessor goes now, at the actual timestep_end, but
      //   only if _picard_max_its>1: 
      //_problem.computeUserObjects(EXEC_CUSTOM, UserObjectWarehouse::POST_AUX);
      _problem.execute(EXEC_CUSTOM); // new method
      return;
    }
  }
  
  // For multiple sub-apps you'll need an aggregate PP to collect residuals in
  //    or some way to keep the number of PPs low.
  
  if (his_initial_norm ==0)
  { his_initial_norm = my_current_norm; } // this statement is nearly meaningless,
  // just ensure it won't revert us back to Solution Interruption.
  // In other words, assume the other residual is comparable

  _new_tol = std::min(his_initial_norm*_new_tol_mult, 0.95*my_current_norm);
  // you may want 0.95 to be a parameter for the user to (not) change
  if (_new_tol < _min_abs_tol)
  { _new_tol = _min_abs_tol;}
  _console << "New Abs_Tol = " << _new_tol << std::endl;
  _problem.es().parameters.set<Real> ("nonlinear solver absolute residual tolerance") = _new_tol;


  _time_stepper->step();

  // We know whether or not the nonlinear solver thinks it converged, but we need to see if the executioner concurs
  if (lastSolveConverged())
  {
    _console << COLOR_GREEN << " Solve Converged!" << COLOR_DEFAULT << std::endl;

    if (_picard_max_its <= 1 )
      _time_stepper->acceptStep();

    _solution_change_norm = _problem.relativeSolutionDifferenceNorm();
    
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


// Try to over-ride reseting the sub-apps 
void
ResidualBalanceTransient::takeStep(Real input_dt)
{
  _picard_it = 0;

  _problem.backupMultiApps(EXEC_TIMESTEP_BEGIN);
  _problem.backupMultiApps(EXEC_TIMESTEP_END);
  
  //his_initial_norm_old = getPostprocessorValue("InitialResidual");
  // _console << "Update his_old_initial \n" << std::endl;

  while (_picard_it<_picard_max_its && _picard_converged == false)
  {
    if (_picard_max_its > 1)
    {
      _console << "\nBeginning Picard Iteration " << _picard_it << "\n" << std::endl;

      Real current_norm = _problem.computeResidualL2Norm();

      if (_picard_it == 0) // First Picard iteration - need to save off the initial nonlinear residual
      {
        _picard_initial_norm = current_norm;
        _console << "Initial Picard Norm: " << _picard_initial_norm << '\n';
      }
    }

    // For every iteration other than the first, we need to restore the state of the MultiApps
    if (_picard_it > 0)
    {
      _problem.restoreMultiApps(EXEC_TIMESTEP_BEGIN);
      _problem.restoreMultiApps(EXEC_TIMESTEP_END);
    }

    solveStep(input_dt);

    /*
    // Remove this check, we check at the beginning of solveStep.
    // We could reformulate this check like the other, but I don't think we gain anything.
    if (_picard_max_its > 1)
    {
      _picard_timestep_end_norm = _problem.computeResidualL2Norm(); 
           // +getPostprocessorValue("FinalResidual"); //if timestep_begin apps

      _console << "Picard Norm after TIMESTEP_END MultiApps: " << _picard_timestep_end_norm << '\n';

      Real max_norm = std::max(_picard_timestep_begin_norm, _picard_timestep_end_norm);

      Real max_relative_drop = max_norm / _picard_initial_norm;

      if (max_norm < _picard_abs_tol || max_relative_drop < _picard_rel_tol)
      {
        _console << "Picard converged!" << std::endl;

        _picard_converged = true;
        return;
      }
    }
     */ 

    ++_picard_it;
  }
}

