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

#ifndef AITKENPREDICTOR_H
#define AITKENPREDICTOR_H

#include "Predictor.h"
#include "libmesh/numeric_vector.h"
//#include "MooseVariableBase.h"
//#include "CoupleableMooseVariableDependencyIntermediateInterface.h"

class AitkenPredictor;

template<>
InputParameters validParams<AitkenPredictor>();

/**
 *
 */
class AitkenPredictor : public Predictor
//var  public CoupleableMooseVariableDependencyIntermediateInterface
{
public:
  AitkenPredictor(const InputParameters & parameters);
  virtual ~AitkenPredictor();

  //virtual int order() { return _order; }
  virtual void apply(NumericVector<Number> & sln);
  virtual NumericVector<Number> & solutionPredictor() { return _solution_predictor; }
  //virtual void historyControl();
  bool shouldApply();

protected:
  /*int _order; 
  NumericVector<Number> & _current_old_solution;
  NumericVector<Number> & _older_solution;
  NumericVector<Number> & _oldest_solution;
  NumericVector<Number> & _tmp_previous_solution;*/
  NumericVector<Number> & _tmp_vec1;
  NumericVector<Number> & _tmp_vec2;
  //int & _t_step_old;
  //Real & _dt_older;
  //Real & _dtstorage;
  int _counter;

 /* const VariableValue & _transf_var;
  const VariableValue & _transf_var_old;
  const VariableValue & _transf_var_older; */
};

#endif /* AITKENPREDICTOR_H */
