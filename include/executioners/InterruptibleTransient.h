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

#ifndef INTERRUPTIBLETRANSIENT_H
#define INTERRUPTIBLETRANSIENT_H

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
class InterruptibleTransient;
class TimeStepper;

template<>
InputParameters validParams<InterruptibleTransient>();

/**
 * Transient executioners usually loop through a number of timesteps... calling solve()
 * for each timestep.
 */
class InterruptibleTransient: public Transient //Executioner
{
public:
  /**
   * Constructor
   *
   * @param name The name given to the Executioner in the input file.
   * @param parameters The parameters object holding data for the class to use.
   * @return Whether or not the solve was successful.
   */
  InterruptibleTransient(const InputParameters & parameters);
  virtual ~InterruptibleTransient();

  /**
   * Initialize executioner
   */
  virtual void init();

  /**
   * Do whatever is necessary to advance one step.
   */
  virtual void takeStep(Real input_dt = -1.0);

  /**
   * Do whatever is necessary to advance one step.
   * Slightly different for when we are re-solving 
   * The multi-app. -Jaron
   */
  virtual void re_takeStep(Real input_dt = -1.0);

  /**
   * This is where the solve step is actually incremented.
   */
  virtual void incrementStepOrReject();

  // Jaron did this. Don't move forward if it's been solved before
  virtual void re_incrementStepOrReject(); 

  virtual void endStep(Real input_time = -1.0);

protected:
  /**
   * This should execute the solve for one timestep.
   */
  virtual void solveStep(Real input_dt = -1.0);

  /**
   * This should execute the RE-solve for one timestep. 
   */
  virtual void re_solveStep(Real input_dt = -1.0);

};

#endif //INTERRUPTIBLETRANSIENT_H
