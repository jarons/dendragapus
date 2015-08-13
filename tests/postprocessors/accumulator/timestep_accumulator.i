[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 15
  ny = 15
[]

[Variables]
  [./u]
  [../]
[]

[Kernels]
  [./u_dot]
    type = TimeDerivative
    variable = u
  [../]
  [./diff]
    type = Diffusion
    variable = u
  [../]
[]

[BCs]
  [./fixed_left]
    type = DirichletBC
    variable = u
    boundary = left
    value = 1
  [../]
  [./slope_right]
    type = NeumannBC
    variable = u
    boundary = right
    value = 0.2
  [../]
[]

[Postprocessors]
  active = 'NNLs total_NNLs'
  [./initial_residual]
    type = InitialResidual
  [../]
  [./NNLs]
    type = NumNonlinearIterations
    accumulate_over_step = true
  [../]
  [./total_NNLs]
    type = AccumulatorPostprocessor
    execute_on = timestep_end
    value = NNLs
  [../]
[]

[Executioner]
  # petsc_options_iname = '-pc_type -pc_hypre_type'
  # petsc_options_value = 'hypre boomeramg'
  type = Transient
  num_steps = 3
  l_max_its = 50
  solve_type = PJFNK
  picard_max_its = 1
  picard_abs_tol = 1e-10
  picard_rel_tol = 1e-10
  nl_abs_tol = 1e-20
  nl_rel_tol = 1e-08
[]

[Outputs]
  exodus = true
  solution_history = true
  output_on = 'timestep_end final'
  [./console]
    type = Console
    perf_log = true
    outlier_variable_norms = false
    output_on = 'timestep_end nonlinear failed'
  [../]
  [./left_out]
    file_base = lefto
    type = Exodus
  [../]
[]

