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
  active = 'Right_fixed Left_fixed'
  [./gradRobin]
    type = gradRobinBC
    variable = T_left
    boundary = right
    grad_v = AV_dT_L
    v = T1fromRight
    Alpha = 0.5
  [../]
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

[Problem]
  type = FEProblem
  use_legacy_uo_initialization = false
  use_legacy_uo_aux_computation = false
[]

[Executioner]
  type = Transient
  num_steps = 1
  l_max_its = 50
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
  dtmin = 2e-20
  picard_abs_tol = 1e-10
  nl_abs_tol = 1e-14
  nl_rel_step_tol = 1e-20
  nl_abs_step_tol = 1e-20
  line_search = basic
[]

[Outputs]
  exodus = true
  solution_history = true
  execute_on = 'timestep_end final'
[]

[MultiApps]
  [./GetRight]
    type = InterruptibleTransientMultiApp
    app_type = DendragapusApp
    execute_on = nonlinear
    positions = '1.0 0 0'
    input_files = SI_right.i
  [../]
[]
