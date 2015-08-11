#ifndef DENDRAGAPUSAPP_H
#define DENDRAGAPUSAPP_H

#include "MooseApp.h"

class DendragapusApp;

template<>
InputParameters validParams<DendragapusApp>();

class DendragapusApp : public MooseApp
{
public:
  DendragapusApp(const InputParameters & parameters);
  virtual ~DendragapusApp();

  static void registerApps();
  static void registerObjects(Factory & factory);
  static void associateSyntax(Syntax & syntax, ActionFactory & action_factory);
};

#endif /* DENDRAGAPUSAPP_H */
