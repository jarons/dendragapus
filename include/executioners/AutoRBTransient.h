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

#ifndef AUTORBTRANSIENT_H
#define AUTORBTRANSIENT_H

#include "Executioner.h"
#include "FEProblem.h"
#include "Transient.h"

// LibMesh includes
#include "libmesh/mesh_function.h"
#include "libmesh/parameters.h"

// System includes
#include <string>
#include <fstream>

// Forward Declarations
class AutoRBTransient;
class TimeStepper;

template<>
InputParameters validParams<AutoRBTransient>();

/**
 * Transient executioners usually loop through a number of timesteps... calling solve()
 * for each timestep.
 */
class AutoRBTransient: public Transient //Executioner
{
public:
  /**
   * Constructor
   *
   * @param name The name given to the Executioner in the input file.
   * @param parameters The parameters object holding data for the class to use.
   * @return Whether or not the solve was successful.
   */
  AutoRBTransient(const InputParameters & parameters);
  virtual ~AutoRBTransient();

  virtual void endStep(Real input_time = -1.0);

  /**
   * Get the number of Picard iterations performed
   * @return Number of Picard iterations performed
   */
  //Because this returns the number of Picard iterations, rather than the current
  //iteration count (which starts at 0), increment by 1.
  Real numPicardIts() { return _picard_it+1; }

  virtual void takeStep(Real input_dt = -1.0); // undoing multi-app reseting

protected:
  /**
   * This should execute the solve for one timestep.
   */
  virtual void solveStep(Real input_dt = -1.0);
  
  Real _new_tol_mult;
  
  PostprocessorValue _new_tol; 
  
  PostprocessorValue _his_final_norm;

  Real _min_abs_tol;
private:
  Real _current_norm; 
  Real _current_norm_old; //this is a bad name
  Real _his_normalizer;
  Real _last_time;
  Real _residual_normalizer;
  Real _spectral_radius; //estimate of the spectral radius
  Real _my_current_norm; // the norm from this app only
  bool _adjust_initial_norm;
  Real _his_initial_norm;
  Real _his_initial_norm_old;
};

#endif //AUTORBTRANSIENT_H
