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
  [./num_pic_its]
    type = NumPicardIterations
  [../]
  [./num_nl_its]
    type = NumNonlinearIterations
    accumulate_over_step = true
  [../]
[]

[Problem]
  type = FEProblem
  use_legacy_uo_initialization = false
  use_legacy_uo_aux_computation = false
[]

[Executioner]
  type = ResidualBalanceTransient
  num_steps = 3
  l_max_its = 50
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
  picard_max_its = 100
  picard_abs_tol = 1e-10
  nl_rel_tol = 0.1
  tol_mult = 1.0e-20 # keep this from acting
  InitialResidual = His_Residual_PP
  FinalResidual = his_final_residual
[]

[Outputs]
  output_on = 'timestep_end final'
  execute_on = TIMESTEP_END # INITIAL 
  [./console]
    type = Console
    perf_log = true
    outlier_variable_norms = false
    output_on = 'timestep_end nonlinear failed'
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
    input_files = rn_sub.i
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
[]

