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

[Problem]
  type = FEProblem
  use_legacy_uo_initialization = false
[]

[Executioner]
  type = InterruptibleTransient
  num_steps = 1
  solve_type = PJFNK
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
  nl_abs_tol = 1e-14
  nl_rel_tol = 1e-8
  nl_rel_step_tol = 1e-20
  nl_abs_step_tol = 1e-20
[]

[Outputs]
  #
  exodus = true
  output_final = true
[]

