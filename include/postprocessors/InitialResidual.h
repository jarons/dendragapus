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

#ifndef INITIALRESIDUAL_H
#define INITIALRESIDUAL_H

#include "GeneralPostprocessor.h"

//Forward Declarations
class InitialResidual;

template<>
InputParameters validParams<InitialResidual>();

class InitialResidual : public GeneralPostprocessor
{
public:
  InitialResidual(const std::string & name, InputParameters parameters);

  virtual void initialize() {}
  virtual void execute(); 

  /**
   * This should return the Initial nonlinear residual.
   */
  virtual Real getValue();
  
  virtual void finalize();

protected:
  processor_id_type _root_id;
  Real _value;
};

#endif // INITIALRESIDUAL_H
