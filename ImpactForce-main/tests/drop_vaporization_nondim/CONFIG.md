# Test Suite Configuration

## Nondimensional Parameters

### Fixed Parameters

These parameters are **FIXED** across all test cases:

```yaml
material_ratios:
  eta: 0.000623              # rho_g / rho_l = 0.597 / 958.4
  mu_ratio: 0.045            # mu_g / mu_l

nondimensional_groups:
  We: 1.5                    # Weber number
  Sigma_star: 0.6666666667   # = 1 / We
  St: 0.1                    # Stefan number
  Pr: 1.0                    # Prandtl number

scales:
  D: 1.0                     # Characteristic length
  U_inf: 1.0                 # Free-stream velocity
  rho_l: 1.0                 # Liquid density reference
  t_ref: 1.0                 # = D / U_inf
```

### Variable Parameters

**Reynolds number sweep**: 25 values linearly spaced from 22 to 200

For each Re:
```
mu_g_star = eta / Re
mu_l_star = mu_g_star / mu_ratio
```

## Geometry Configuration

```yaml
geometry:
  domain_L0: 8.0             # Domain size (8 × 8)
  drop_radius_R0: 0.5        # Drop radius
  drop_center_x: 1.5         # Drop center x-coordinate
  drop_center_y: 0.0         # Drop center y-coordinate (on axis)
  R_inf: 0.625               # Temperature smoothing radius (1.25 × R0)
```

## Mesh Configuration

```yaml
mesh:
  Lmax: 12                   # Maximum refinement level
  Lmin: 2                    # Minimum refinement level
  base_grid: 64              # Base grid resolution (64 × 64)
  dx_min: 0.001953125        # = 1 / 512 = 1 / (2^Lmax)

adaptive_refinement:
  method: wavelet
  error_tolerances:
    f_VOF: 0.01              # VOF field
    u_velocity: 0.01         # Velocity field
    T_temperature: 0.02      # Temperature field
```

## Time Configuration

```yaml
time:
  CFL: 0.2                   # CFL number
  t_end_paper: 0.16          # End time in paper time t*
  t_end_solver: null         # Computed: t_end_paper / sqrt(rho_l/rho_g)

time_scales:
  t_plus: t * U_inf / D      # Solver time (already nondimensional)
  t_star: t_plus * sqrt(rho_l / rho_g)  # Paper time
```

## Boundary Conditions

```yaml
boundary_conditions:
  left:                      # Inflow
    u.n: dirichlet(1.0)      # u_x = 1 (U_inf)
    u.t: dirichlet(0.0)      # u_y = 0
    T: dirichlet(1.0)        # T* = 1 (free stream)
    f: dirichlet(0.0)        # f = 0 (gas phase)

  right:                     # Outflow
    u.n: neumann(0)          # du/dx = 0
    p: dirichlet(0)          # p = 0 (pressure outlet)
    T: neumann(0)            # dT/dx = 0

  axis:                      # Axisymmetric
    automatic: true          # Handled by axi.h
```

## Initial Conditions

```yaml
initial_conditions:
  volume_fraction:
    sphere:
      center: [1.5, 0.0]
      radius: 0.5
      inside: 1.0            # Liquid
      outside: 0.0           # Gas

  temperature:
    type: radial_profile
    r_center: [1.5, 0.0]
    profile:
      - r <= 0.5: T* = 0.0   # Saturation
      - r >= 0.625: T* = 1.0 # Free stream
      - 0.5 < r < 0.625:     # Linear interpolation (Eq. 53)
          T* = (r - 0.5) / (0.625 - 0.5)

  velocity:
    u_x: 1.0                 # Uniform inflow
    u_y: 0.0
```

## Output Configuration

```yaml
output:
  directory: results/Re_{Re:.2f}_We_{We:.2f}

  diagnostics:
    interval: 0.01           # Output every 0.01 time units
    files:
      - liquid_volume_history.txt
      - nusselt_history.txt
      - frontal_area_history.txt

  snapshots:
    vtk:
      interval: 0.01
      format: vtk
      fields: [f, T, u.x, u.y, p]

    dump:
      interval: 0.05
      format: basilisk_binary

  logging:
    frequency: every_timestep
    file: log.txt
```

## Physical Properties (for reference)

These are the **physical** properties from which the material ratios are derived:

```yaml
physical_properties:
  liquid_water:
    rho: 958.4               # kg/m³
    mu: 2.8e-4               # Pa·s
    k: 0.679                 # W/(m·K)
    cp: 4216.0               # J/(kg·K)

  gas_steam:
    rho: 0.597               # kg/m³
    mu: 1.26e-5              # Pa·s
    k: 0.025                 # W/(m·K)
    cp: 2030.0               # J/(kg·K)

  interface:
    sigma: 0.0728            # N/m (surface tension)
    h_lg: 2.26e6             # J/kg (latent heat)
    T_sat: 373.15            # K
    T_inf: 600.0             # K
```

## Reynolds Number Sweep

```python
import numpy as np
re_values = np.linspace(22, 200, 25)

# Generates:
# [22.00, 29.42, 36.83, 44.25, 51.67, 59.08, 66.50, 73.92, 81.33,
#  88.75, 96.17, 103.58, 111.00, 118.42, 125.83, 133.25, 140.67,
#  148.08, 155.50, 162.92, 170.33, 177.75, 185.17, 192.58, 200.00]
```

## Validation Criteria

```yaml
validation:
  mass_conservation:
    tolerance: 1e-6          # Relative volume error

  volume_evolution:
    monotonic: true          # Volume should decrease monotonically
    expected_range:
      Re_22: [0.93, 0.97]    # V/V0 at t* = 0.16
      Re_100: [0.83, 0.87]
      Re_200: [0.73, 0.77]

  stability:
    max_cfl: 0.5             # Maximum CFL encountered
    no_blowup: true          # No NaN or Inf values

  nusselt:
    scaling: power_law       # Nu ~ Re^n
    exponent_range: [0.4, 0.6]
```

## Performance

```yaml
performance:
  expected_runtime:
    single_case_Lmax12: 1800  # ~30 min
    single_case_Lmax10: 300   # ~5 min
    full_sweep_25cases: 18000 # ~5 hours (sequential)

  memory:
    Lmax12: 4096             # MB
    Lmax10: 1024             # MB

  recommended:
    quick_test: Lmax=10
    production: Lmax=12
    parallel_jobs: 4
```

## Reference

```yaml
reference:
  paper: "Boyd & Ling 2023"
  title: "Direct numerical simulation of evaporating droplets based on a sharp-interface algebraic VOF approach"
  journal: "Computers & Fluids"
  section: "5.1, Figure 10, Table 2"
  doi: "TBD"
```
