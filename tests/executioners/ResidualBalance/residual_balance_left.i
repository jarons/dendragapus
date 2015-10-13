[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Variables]
  [./T_left]
    block = 0
  [../]
[]

[Kernels]
  active = 'left_diff'
  [./left_diff]
    type = Diffusion
    variable = T_left
    block = 0
  [../]
  [./Convection]
    type = Convection
    variable = T_left
    x = 1
    y = 0
  [../]
[]

[BCs]
  [./Left_fixed]
    type = DirichletBC
    variable = T_left
    boundary = left
    value = 1
  [../]
  [./Right_fixed]
    type = DirichletBC
    variable = T_left
    boundary = right
    value = 3.
  [../]
[]

[Postprocessors]
  [./My_Residual_PP]
    type = InitialResidual
    execute_on = 'nonlinear initial'
  [../]
  [./His_Residual_PP]
    type = Receiver
    default = 0
    execute_on = 'timestep_begin initial'
  [../]
  [./his_final_residual]
    type = Receiver
    default = 0
  [../]
[]

[Problem]
  type = FEProblem
  use_legacy_uo_initialization = false
  use_legacy_uo_aux_computation = false
[]

[Executioner]
  type = ResidualBalanceTransient
  num_steps = 1
  l_max_its = 50
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
  dtmin = 2e-20
  picard_max_its = 100
  picard_abs_tol = 1e-10
  nl_abs_tol = 1e-20
  nl_rel_step_tol = 1e-08
  nl_abs_step_tol = 1e-20
  line_search = basic
  tol_mult = 0.5
  InitialResidual = His_Residual_PP
  FinalResidual = his_final_residual
[]

[Outputs]
  exodus = true
  solution_history = true
  execute_on = TIMESTEP_END # INITIAL 
  [./console]
    type = Console
    perf_log = true
    outlier_variable_norms = false
    execute_on = 'timestep_end nonlinear failed'
  [../]
[]

[MultiApps]
  [./GetRight]
    type = TransientMultiApp
    app_type = DendragapusApp
    execute_on = timestep_end
    positions = '1.0 0 0'
    input_files = residual_balance_right.i
  [../]
[]

[Transfers]
  [./send_Residual]
    type = MultiAppPostprocessorTransfer
    direction = to_multiapp
    multi_app = GetRight
    from_postprocessor = My_Residual_PP
    to_postprocessor = His_Residual_PP
  [../]
  [./get_Residual]
    type = MultiAppPostprocessorTransfer
    direction = from_multiapp
    multi_app = GetRight
    reduction_type = minimum
    from_postprocessor = My_Residual_PP
    to_postprocessor = His_Residual_PP
  [../]
  [./get_final_residual]
    type = MultiAppPostprocessorTransfer
    direction = from_multiapp
    multi_app = GetRight
    reduction_type = minimum
    from_postprocessor = final_residual
    to_postprocessor = his_final_residual
  [../]
[]

