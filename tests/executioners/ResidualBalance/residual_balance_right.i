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
  [./gradRobin]
    # v: Tfront does better
    # Coef -1
    type = gradRobinBC
    variable = Tfront
    boundary = left
    v = T1fromLeft
    grad_v = AV_dT_R
    Alpha = 0.7
  [../]
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
  [./gradTpp]
    type = cSideFluxAverage
    variable = Tfront
    boundary = left
    diffusion_coefficient = 0.1 # -0.1
    execute_on = 'TIMESTEP_END initial'
  [../]
  [./T1_pp]
    type = PointValue
    variable = Tfront
    point = '0 0 0'
    execute_on = 'TIMESTEP_END initial'
  [../]
  [./NL_R]
    type = NumNonlinearIterations
    accumulate_over_step = true
    execute_on = 'TIMESTEP_END initial'
  [../]
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
  # Preconditioned JFNK (default)
  # l_max_its = 50
  # 1e-8
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
  # output_initial = true
  exodus = true
  output_final = true
  [./console]
    # linear_residuals = true
    type = Console
    perf_log = true
    output_postprocessors_on = 'TIMESTEP_END timestep_begin'
  [../]
  [./exod]
    # output_failed = true
    file_base = righto-2D4
    type = Exodus
    output_on = 'timestep_begin failed'
  [../]
[]

