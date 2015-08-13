#include "DendragapusApp.h"
#include "Moose.h"
#include "AppFactory.h"
// #include "ModulesApp.h" Also see Makefile for including modules
//   Allows ModulesApp (two locations in this file)

//important new things
#include "ResidualBalanceTransient.h"
#include "InterruptibleTransient.h"
#include "InitialResidual.h"
#include "InterruptibleTransientMultiApp.h"

template<>
InputParameters validParams<DendragapusApp>()
{
  InputParameters params = validParams<MooseApp>();

  params.set<bool>("use_legacy_uo_initialization") = false;
  params.set<bool>("use_legacy_uo_aux_computation") = false;
  return params;
}

DendragapusApp::DendragapusApp(InputParameters parameters) :
    MooseApp(parameters)
{

  Moose::registerObjects(_factory);
  // ModulesApp::registerObjects(_factory);
  DendragapusApp::registerObjects(_factory);

  Moose::associateSyntax(_syntax, _action_factory);
  // ModulesApp::associateSyntax(_syntax, _action_factory);
  DendragapusApp::associateSyntax(_syntax, _action_factory);
}

DendragapusApp::~DendragapusApp()
{
}

// External entry point for dynamic application loading
extern "C" void DendragapusApp__registerApps() { DendragapusApp::registerApps(); }
void
DendragapusApp::registerApps()
{
  registerApp(DendragapusApp);
}

// External entry point for dynamic object registration
extern "C" void DendragapusApp__registerObjects(Factory & factory) { DendragapusApp::registerObjects(factory); }
void
DendragapusApp::registerObjects(Factory & factory)
{
  registerMultiApp(InterruptibleTransientMultiApp);
  registerPostprocessor(InitialResidual);
  registerExecutioner(ResidualBalanceTransient);
  registerExecutioner(InterruptibleTransient);
}

// External entry point for dynamic syntax association
extern "C" void DendragapusApp__associateSyntax(Syntax & syntax, ActionFactory & action_factory) { DendragapusApp::associateSyntax(syntax, action_factory); }
void
DendragapusApp::associateSyntax(Syntax & syntax, ActionFactory & action_factory)
{
  //nothing here yet
}
