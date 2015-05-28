#include "DendragapusApp.h"
#include "Moose.h"
#include "AppFactory.h"
#include "ModulesApp.h"

template<>
InputParameters validParams<DendragapusApp>()
{
  InputParameters params = validParams<MooseApp>();

  params.set<bool>("use_legacy_uo_initialization") = false;
  params.set<bool>("use_legacy_uo_aux_computation") = false;
  return params;
}

DendragapusApp::DendragapusApp(const std::string & name, InputParameters parameters) :
    MooseApp(name, parameters)
{
  srand(processor_id());

  Moose::registerObjects(_factory);
  ModulesApp::registerObjects(_factory);
  DendragapusApp::registerObjects(_factory);

  Moose::associateSyntax(_syntax, _action_factory);
  ModulesApp::associateSyntax(_syntax, _action_factory);
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
}

// External entry point for dynamic syntax association
extern "C" void DendragapusApp__associateSyntax(Syntax & syntax, ActionFactory & action_factory) { DendragapusApp::associateSyntax(syntax, action_factory); }
void
DendragapusApp::associateSyntax(Syntax & syntax, ActionFactory & action_factory)
{
}
