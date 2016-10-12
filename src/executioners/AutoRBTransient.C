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
#include "AutoRBTransient.h"

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
InputParameters validParams<AutoRBTransient>()
{
  InputParameters params = validParams<Transient>(); 
  params.addParam<Real>("tol_mult",0.1,"0<this<1. (Other residual)*this=(new_abs_tol)"); 
  params.addRequiredParam<PostprocessorName>("InitialResidual", "The name of the InitialResidual postprocessor you are trying to get.");
  params.addParam<PostprocessorName>("FinalResidual",0.0, "The name of the Residual postprocessor you are trying to get.");
  params.addParam<Real>("nl_abs_tol",1e-50,"Set this to the same as nl_abs_tol"); //this overrides other nl_abs_tol
  
  params.addParamNamesToGroup("nl_abs_tol", "Solver"); // put it back into the Solver tab
  return params;
}

AutoRBTransient::AutoRBTransient(const InputParameters & parameters) :
    Transient(parameters), 
    _new_tol_mult(getParam<Real>("tol_mult")),
    _new_tol(getPostprocessorValue("InitialResidual")),
    _his_final_norm(getPostprocessorValue("FinalResidual")),
    _min_abs_tol(getParam<Real>("nl_abs_tol")),
    _current_norm_old(0)
    //his_initial_norm_old(-1.0)
{
  //*#*//  These changes set _spectral_radius only for the very beginning.
    // Afterwards, it is carried over between timesteps.
    // This did not work well, because solvers don't start out asymptotic
    //   like they finish.
  //*#*//_spectral_radius = pow(_new_tol_mult, 0.5);
}

AutoRBTransient::~AutoRBTransient()
{
}

void
AutoRBTransient::solveStep(Real input_dt)
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

  // You could evaluate/store _his_initial_norm_old  here
  
  _problem.execTransfers(EXEC_TIMESTEP_BEGIN);
  _multiapps_converged = _problem.execMultiApps(EXEC_TIMESTEP_BEGIN, _picard_max_its == 1);

  if (!_multiapps_converged)
    return;

  preSolve();
  _time_stepper->preSolve();

  _problem.timestepSetup();
  
  _problem.execute(EXEC_TIMESTEP_BEGIN);

  // Perform output for timestep begin
  _problem.outputStep(EXEC_TIMESTEP_BEGIN);

  // Update warehouse active objects
  _problem.updateActiveObjects();

/*
  if (_picard_it == 0) //first iteration of timestep
  {
    Real _residual_normalizer = _problem.computeResidualL2Norm(); //very first residual  
    _console << "Residual normalizer: " << _residual_normalizer << '\n';
  // TODO: account for progress towards tolerance:
  // _effective_tol needs to be recomputed at each timestep
  // Real _effective_tol = std::max(_min_abs_tol, _residual_normalizer * getParam<Real>("nl_rel_tol"));
  // Real _residual_normalizer = _problem.computeResidualL2Norm() * _effective_tol;

    _my_current_norm = 1.0; 
    // equivalent to: _problem.computeResidualL2Norm() / _residual_normalizer;
  }
  else  // not first iteration of timestep
  {
    // the norm from this app only, divided by the very first residual
    _my_current_norm = _problem.computeResidualL2Norm() / _residual_normalizer; 
  }
*/

  _his_initial_norm_old = _his_initial_norm;
  _his_initial_norm = getPostprocessorValue("InitialResidual");
    //for sub-app@timestep_begin: his_initial_norm should be zero at each timestep
  
  // if this is sub app and its the initial_residual wasn't updated 
      // or use his_initial_norm_old
  //  On the first iteration Old should not match _his_initial if sub-app comes
  //    second (timestep_end).
  //if (_picard_max_its==1 && _his_initial_norm == getPostprocessorValueOld("InitialResidual"))
  if (_picard_max_its==1 && _his_initial_norm == _his_initial_norm_old)
  {
    _his_initial_norm = 0;
  }
  //sub-app and first iteration of timestep:
  //if (_picard_max_its==1 && _residual_normalizer==0) //
  if (_picard_max_its==1 && _his_initial_norm_old < _his_initial_norm) //assuming monotone
    _residual_normalizer = _problem.computeResidualL2Norm();

  _my_current_norm = _problem.computeResidualL2Norm() / _residual_normalizer;
  
  _console << "_my_current_norm= " << _my_current_norm << '\n';
  
  _his_final_norm = getPostprocessorValue("FinalResidual");
  if (_picard_max_its > 1)
  {    
      // is there a way to get his_final_norm without a postprocessor transfer?
      //std::vector<MultiApp *> multi_apps = _problem._multi_apps(EXEC_TIMESTEP_END)[0].all();
          // unfortunately _multi_apps is protected
      //Real his_last_norm = multi_apps[i]._subproblem.finalNonlinearResidual();
      //_console << "Multi-app end residual" << his_final_norm << std::endl; // did it work?

    if (_picard_it == 0) // First Picard iteration - need to save off the initial nonlinear residual
    {
      _current_norm = _my_current_norm;  
      //his_initial_norm_old) //EXEC_TIMESTEP_END
      //if (_his_initial_norm == getPostprocessorValueOld("InitialResidual")) 
      if (_his_initial_norm == _his_initial_norm_old) 
        _his_initial_norm = 0; 
      // set his_initial_norm to 0 so that we can start a new timestep properly
      
      _picard_initial_norm = _current_norm + _his_initial_norm; 
      //_console << "Initial Picard Norm: " << _picard_initial_norm << '\n';
      if (_his_initial_norm == 0)
        _adjust_initial_norm = true;
      _spectral_radius = pow(_new_tol_mult, 0.5);//*#*//
    }
    else
    {
      _current_norm_old = _current_norm;
      _current_norm = _my_current_norm + _his_final_norm; 
      //@^&// These changes smooth out the change in _spectral_radius
      //@^&//_spectral_radius = _current_norm / _current_norm_old;
      //@^&//_spectral_radius = 0.5 * (_current_norm / _current_norm_old + _spectral_radius);
      _spectral_radius = pow(_current_norm / _current_norm_old * _spectral_radius, 0.5);
      _console << "Current Picard Norm: " << _current_norm << '\n';
    }

    if (_picard_it == 1 && _adjust_initial_norm == true)
    {
      _picard_initial_norm = _picard_initial_norm + _his_initial_norm;
      _console << "Adjusted Initial Picard Norm: " << _picard_initial_norm << '\n';
      //@^&//_spectral_radius = _current_norm / _picard_initial_norm;
      //@^&//_spectral_radius = 0.5 * (_current_norm / _picard_initial_norm + _spectral_radius);
      _spectral_radius = pow(_current_norm / _picard_initial_norm * _spectral_radius, 0.5);
    }

    Real _relative_drop = _current_norm / _picard_initial_norm;

    if (_current_norm < _picard_abs_tol || _relative_drop < _picard_rel_tol)
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
  else // this is the sub-app
  {
      // If this is the first Picard iteration of the timestep
      //   Second condition ensures proper treatment the very first time
      if ( _his_initial_norm == 0 || _current_norm_old == 0 )
      {
        _spectral_radius = pow(_new_tol_mult, 0.5);//*#*//
        _current_norm = _his_final_norm + _my_current_norm;
      }
      else
      {
        _current_norm_old = _current_norm;
        _current_norm = _his_final_norm + _my_current_norm;
        //@^&//_spectral_radius = _current_norm / _current_norm_old;
        //@^&//_spectral_radius = 0.5 * (_current_norm / _current_norm_old + _spectral_radius);
        _spectral_radius = pow(_current_norm / _current_norm_old * _spectral_radius, 0.5);
      }
  }
  
  // For multiple sub-apps you'll need an aggregate PP to collect residuals in
  //    or some way to keep the number of PPs low.
  
  if (_his_initial_norm == 0)
    _his_initial_norm = _my_current_norm; 
  //assume the other residual is comparable to this one

  _console << "_his_initial_norm = " << _his_initial_norm << "  rho=" << _spectral_radius << std::endl;

  //_new_tol = std::min(_his_initial_norm*_new_tol_mult, 0.95*_my_current_norm);
  _new_tol = std::min(_his_initial_norm * _spectral_radius * _spectral_radius
      * _residual_normalizer, 0.95 * _my_current_norm * _residual_normalizer);
  // you may want 0.95 to be a parameter for the user to (not) change
  if (_new_tol < _min_abs_tol)
    _new_tol = _min_abs_tol;
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



void
AutoRBTransient::endStep(Real input_time)
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
    /* Remove this to match 9/14/15 update to Transient.C
    if (_picard_max_its > 1)
    {
      _problem.advanceMultiApps(EXEC_TIMESTEP_BEGIN);
      _problem.advanceMultiApps(EXEC_TIMESTEP_END);
    }

		//Jaron was here. //Is this needed?
    _problem.advanceMultiApps(EXEC_NONLINEAR);
     */

    //output
    if (_time_interval && (_time + _timestep_tolerance >= _next_interval_output_time))
      _next_interval_output_time += _time_interval_output_interval;
   }
}

// Try to over-ride reseting the sub-apps 
void
AutoRBTransient::takeStep(Real input_dt)
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

      
      //Real _current_norm = _problem.computeResidualL2Norm() / _residual_normalizer;

      if (_picard_it == 0) // First Picard iteration - need to save off the initial nonlinear residual
      {
        _residual_normalizer = _problem.computeResidualL2Norm(); //first residual of timestep
        _picard_initial_norm = 1.0; //_problem.computeResidualL2Norm() / _residual_normalizer;
        _console << "Initial Picard Norm: " << _picard_initial_norm << '\n';
      }
    }

    // For every iteration other than the first, we need to restore the state of the MultiApps
    if (_picard_it > 0)
    {
      // This is supposed to regain the previous behavior
      //_problem.restoreMultiApps(EXEC_TIMESTEP_BEGIN);
      //_problem.restoreMultiApps(EXEC_TIMESTEP_END);
    }

    solveStep(input_dt);

    /*
    // Remove this check, we check at the beginning of solveStep.
    // We could reformulate this check like the other, but I don't think we gain anything.
    if (_picard_max_its > 1)
    {
      _picard_timestep_end_norm = _problem.computeResidualL2Norm();

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

