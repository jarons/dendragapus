[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Variables]
  [./T_left]
    block = 0
    scaling = 10000 #this should demonstrate benefit of normalization
  [../]
[]

[AuxVariables]
  [./v]
  [../]
[]

[Kernels]
  [./left_diff]
    type = Diffusion
    variable = T_left
    block = 0
  [../]
  [./T_dot]
    type = TimeDerivative
    variable = T_left
  [../]
  [./CoupledForce]
    type = CoupledForce
    variable = T_left
    v = v
  [../]
[]

[BCs]
  active = 'Left_fixed'
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
  [./Num_NL_master]
    type = NumNonlinearIterations
    accumulate_over_step = true
  [../]
  [./Num_Picard]
    type = NumPicardIterations
  [../]
  [./my_final_residual]
    type = Residual
    execute_on = 'nonlinear timestep_end'
  [../]
[]

[Problem]
  type = FEProblem
  use_legacy_uo_initialization = false
  use_legacy_uo_aux_computation = false
[]

[Executioner]
  type = AutoRBTransient
  num_steps = 3
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
  adapt_tol_mult = false # revert to normalized RB
  InitialResidual = His_Residual_PP
  FinalResidual = his_final_residual
[]

[Outputs]
  solution_history = true
  execute_on = 'INITIAL TIMESTEP_END final'
  [./console]
    type = Console
    perf_log = true
    outlier_variable_norms = false
  [../]
  [./exodus]
    type = Exodus
    output_nonlinear = true
    execute_on = TIMESTEP_END # INITIAL 
  [../]
[]

[MultiApps]
  [./sub]
    type = TransientMultiApp
    app_type = DendragapusApp
    execute_on = timestep_end
    positions = '0 0 0'
    input_files = autoRB_nrb_sub.i
  [../]
[]

[Transfers]
  [./send_Residual]
    type = MultiAppPostprocessorTransfer
    direction = to_multiapp
    multi_app = sub
    from_postprocessor = My_Residual_PP
    to_postprocessor = His_Residual_PP
  [../]
  [./get_Residual]
    type = MultiAppPostprocessorTransfer
    direction = from_multiapp
    multi_app = sub
    reduction_type = minimum
    from_postprocessor = My_Residual_PP
    to_postprocessor = His_Residual_PP
  [../]
  [./get_final_residual]
    type = MultiAppPostprocessorTransfer
    direction = from_multiapp
    multi_app = sub
    reduction_type = minimum
    from_postprocessor = final_residual
    to_postprocessor = his_final_residual
  [../]
  [./get_T]
    type = MultiAppMeshFunctionTransfer
    direction = from_multiapp
    multi_app = sub
    source_variable = T_right
    variable = v
  [../]
  [./send_final_norm]
    type = MultiAppPostprocessorTransfer
    direction = to_multiapp
    multi_app = sub
    from_postprocessor = my_final_residual
    to_postprocessor = his_final_residual
  [../]
[]

