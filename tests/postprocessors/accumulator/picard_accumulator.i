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
  [./total_Pic_its]
    type = AccumulatorPostprocessor
    execute_on = custom
    value = Pic_its
  [../]
  [./Pic_its]
    type = NumPicardIterations
  [../]
  [./initial_residual]
    type = InitialResidual
  [../]
[]

[Executioner]
  type = ResidualBalanceTransient
  num_steps = 3
  l_max_its = 50
  solve_type = PJFNK
  # petsc_options_iname = '-pc_type -pc_hypre_type'
  # petsc_options_value = 'hypre boomeramg'
  picard_max_its = 20
  picard_abs_tol = 1e-10
  picard_rel_tol = 1e-08
  nl_abs_tol = 1e-20
  nl_rel_tol = 1e-08 #0.5 # ensure that we do Picard Iterations
  tol_mult = 0.5
  InitialResidual = initial_residual
[]

[Outputs]
  exodus = true
  solution_history = true
  execute_on = 'timestep_end final'
  [./console]
    type = Console
    perf_log = true
    outlier_variable_norms = false
    execute_on = 'timestep_end nonlinear failed'
  [../]
  [./left_out]
    file_base = lefto
    type = Exodus
  [../]
[]
