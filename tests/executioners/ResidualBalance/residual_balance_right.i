[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Variables]
  [./T_right]
  [../]
[]

[Kernels]
  [./right_diff]
    type = Diffusion
    variable = T_right
    block = 0
  [../]
[]

[BCs]
  active = 'Fixed_left Right_fixed'
  [./Right_fixed]
    type = DirichletBC
    variable = T_right
    boundary = right
    value = 5
  [../]
  [./Fixed_left]
    type = DirichletBC
    variable = T_right
    boundary = left
    value = -1
  [../]
[]

[Postprocessors]
  active = 'His_Residual_PP My_Residual_PP final_residual'
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
[]

[Problem]
  type = FEProblem
  use_legacy_uo_initialization = false
[]

[Executioner]
  type = ResidualBalanceTransient
  num_steps = 1
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
  nl_abs_tol = 1e-20
  nl_rel_step_tol = 1e-08
  nl_abs_step_tol = 1e-20
  tol_mult = 0.5
  InitialResidual = His_Residual_PP
[]

[Outputs]
  #
  exodus = true
  output_final = true
  [./console]
    # linear_residuals = true
    type = Console
    perf_log = true
    output_postprocessors_on = 'TIMESTEP_END timestep_begin'
  [../]
[]

