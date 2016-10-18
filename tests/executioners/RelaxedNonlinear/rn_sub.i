[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Variables]
  [./T_right]
    initial_condition = 5
  [../]
[]

[Kernels]
  [./right_diff]
    type = Diffusion
    variable = T_right
    block = 0
  [../]
  [./T_dot]
    type = TimeDerivative
    variable = T_right
  [../]
[]

[BCs]
  [./Right_fixed]
    type = DirichletBC
    variable = T_right
    boundary = right
    value = 5
  [../]
  [./Slope_left]
    type = NeumannBC
    variable = T_right
    boundary = left
    value = -1
  [../]
[]

[Postprocessors]
  [./His_Residual_PP]
    type = Receiver
    default = 0
    execute_on = 'TIMESTEP_begin initial'
  [../]
  [./My_Residual_PP]
    type = InitialResidual
    execute_on = 'nonlinear initial'
  [../]
  [./final_residual]
    type = Residual
  [../]
  [./num_nl_its]
    type = NumNonlinearIterations
    accumulate_over_step = true
  [../]
[]

[Problem]
  type = FEProblem
  use_legacy_uo_initialization = false
[]

[Executioner]
  type = Transient
  num_steps = 1
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
  nl_rel_tol = 0.1
  tol_mult = 1.0e-20
  InitialResidual = His_Residual_PP
[]

[Outputs]
  # output_initial = true
  output_final = true
  execute_on = TIMESTEP_END # INITIAL 
  [./console]
    # linear_residuals = true
    type = Console
    perf_log = true
    output_postprocessors_on = 'TIMESTEP_END timestep_begin'
  [../]
  [./Exodus]
    type = Exodus
    output_nonlinear = true
    execute_on = TIMESTEP_END # INITIAL 
  [../]
[]

