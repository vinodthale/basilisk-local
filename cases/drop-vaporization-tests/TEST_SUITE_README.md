# Drop Vaporization Test Suite

## Overview

This test suite provides comprehensive implementations of drop vaporization with phase change, including:

1. **Axisymmetric Drop Vaporization** - Reproducing **Figure 10** from Boyd & Ling 2023 (Computers and Fluids)
2. **Aerodynamic Breakup with Vaporization** - Advanced test cases for drops undergoing aerodynamic breakup while vaporizing

The test suite includes:
- **YAML equation specification** (`equations.yaml`) - Machine-readable format of all governing equations
- **Complete solvers** - Full 2D axisymmetric solvers based on ImpactForce-main structure
- **Aerodynamic breakup tests** - Stefan problem, bubble growth, film boiling, and sucking problem
- **Constants header** (`constants_vaporization.h`) - Nondimensional parameters and configuration
- **Test scripts** - Compilation and parameter sweep automation

## Key Features

### ✅ Complete Nondimensional Formulation

All equations are in nondimensional form with proper scaling:

- **Length scale**: D₀ (drop diameter) = 1.0
- **Velocity scale**: U∞ (free-stream) = 1.0
- **Density scale**: ρₗ (liquid) = 1.0
- **Time scale**: t_ref = D₀/U∞

### ✅ Governing Physics

1. **Two-phase Navier-Stokes** with density/viscosity jumps
2. **VOF interface tracking** with evaporation source terms
3. **Energy equation** with conduction and latent heat sink
4. **Stefan condition** for evaporative mass flux

### ✅ Based on ImpactForce-main Structure

The solver follows the proven architecture from `ImpactForce-main/Bdropimpact.c`:
- Modular constants header
- Command-line parameter parsing
- Timing and diagnostics
- Adaptive mesh refinement
- Restart capability

## File Structure

```
cases/drop-vaporization-tests/
├── equations.yaml                          # Complete equation specification
├── TEST_SUITE_README.md                    # This file
├── README.md                               # User documentation
├── IMPLEMENTATION_NOTES.md                 # Development notes
├── src/
│   ├── drop_vaporization_nondim.c         # Main solver (FULL TEST SUITE)
│   ├── constants_vaporization.h            # Constants and configuration
│   ├── fig10_drop_vaporization.c          # Alternative version
│   ├── fig10_drop_vaporization_v2.c       # Development version
│   ├── drop_vap.c                         # Simple test version
│   ├── bubble_growth.c                    # Aerodynamic breakup: bubble growth test
│   ├── stefan_problem.c                   # Aerodynamic breakup: Stefan problem
│   ├── film_boiling.c                     # Aerodynamic breakup: film boiling test
│   ├── sucking_problem.c                  # Aerodynamic breakup: sucking problem
│   ├── aerodynamic_breakup_Makefile.txt   # Makefile for aerodynamic breakup tests
│   └── aerodynamic-breakup-headers/       # Headers for aerodynamic breakup tests
│       ├── 01_vaporization/               # Vaporization models and functions
│       ├── common_functions/              # Common utility functions
│       ├── outputs/                       # Output and logging functions
│       ├── properties/                    # Material properties
│       ├── LS_funcs/                      # Level set functions
│       ├── post_processing/               # Post-processing utilities
│       └── theory_evap/                   # Theoretical evaporation models
├── scripts/
│   ├── compile.sh                         # Compilation script
│   ├── run_single_test.sh                 # Run one case
│   ├── run_parameter_sweep.sh             # Run all 25 cases
│   └── visualize_results.py               # Visualization
├── docs/
│   └── aerodynamic-breakup/               # Aerodynamic breakup documentation
│       ├── README.md                      # Aerodynamic breakup overview
│       ├── A consistent volume-of-fluid approach...pdf  # Reference paper
│       └── images/                        # Documentation images
├── build/                                  # Build directory (created on compile)
└── results/                                # Results directory (created on run)
```

## Equations Specification (equations.yaml)

The `equations.yaml` file provides a complete machine-readable specification including:

### 1. Nondimensionalization
- Reference scales
- Dimensionless groups (Re, We, St, Pr)
- Material ratios (density, viscosity, thermal conductivity)

### 2. Governing Equations
- **Continuity**: Mass conservation with phase change
- **Momentum**: Navier-Stokes with surface tension
- **VOF**: Interface advection with evaporation source
- **Energy**: Temperature diffusion with latent heat sink

### 3. Interface Conditions
- **Stefan condition**: ṁ* = -(1/(St Re Pr)) |∇T*|_interface
- Temperature continuity
- Velocity jump due to mass flux

### 4. Boundary Conditions
- Left: Inflow (u=1, T=1, f=0)
- Right: Outflow (p=0, ∂u/∂x=0, ∂T/∂x=0)
- Top: Far-field
- Axis: Axisymmetric (r=0)

### 5. Initial Conditions
- Spherical drop (R₀ = 0.5, center at x = 1.5)
- Temperature profile (Eq. 53 with R_inf = 1.25 R₀)
- Uniform flow (u = 1.0)

### 6. Numerical Methods
- Adaptive mesh refinement (AMR)
- VOF interface tracking
- Implicit diffusion solver
- Multigrid pressure solver

## Test Suite Components

### Main Solver: `drop_vaporization_nondim.c`

Complete implementation with all features:

```c
// Nondimensional parameters (command-line configurable)
Re ∈ [22, 200]    // Reynolds number (swept parameter)
We = 1.5          // Weber number (fixed)
St = 0.1          // Stefan number (fixed)
Pr = 1.0          // Prandtl number (fixed)

// Material ratios (FIXED from physical properties)
η = ρ_g/ρ_l = 0.000623
μ_ratio = 0.045
k_ratio = 0.0368
cp_ratio = 0.4815
```

**Key Events:**
- `init`: Initialize VOF, temperature, velocity fields
- `properties`: Set phase-dependent thermal diffusivity
- `evaporation`: Compute mass flux via Stefan condition
- `tracer_diffusion`: Solve energy equation with latent heat
- `adapt`: Wavelet-based AMR on f, u, T
- `volume_tracking`: Monitor drop shrinkage
- `outputfiles`: Save snapshots and diagnostics

### Constants Header: `constants_vaporization.h`

Provides:
- Nondimensional parameter definitions
- CFD values structure
- Command-line argument parsing
- Time calculation utilities
- All based on ImpactForce-main pattern

### Aerodynamic Breakup Tests

Four test cases for validating vaporization models in complex flow scenarios:

1. **`stefan_problem.c`**: Classic Stefan problem - 1D phase change with moving interface
   - Validates basic evaporation physics
   - Analytical solution available for comparison
   - Tests temperature gradient computation at interface

2. **`bubble_growth.c`**: Bubble growth in superheated liquid
   - Tests mass transfer from liquid to vapor
   - Spherically symmetric evaporation
   - Validates pressure-temperature coupling

3. **`film_boiling.c`**: Film boiling on hot surface
   - Complex vapor layer formation
   - Tests evaporation under high heat flux
   - Relevant for droplet impact on hot surfaces

4. **`sucking_problem.c`**: Interface with evaporative mass flux
   - Tests velocity jump conditions across interface
   - Validates momentum coupling with phase change
   - Important for aerodynamic breakup scenarios

These tests use the comprehensive header library in `src/aerodynamic-breakup-headers/` which includes:
- Advanced vaporization models (`01_vaporization/`)
- Temperature-phase change coupling (`temperature-phase-change.h`)
- Interface tracking functions (`interface_functions.h`)
- Material property correlations (`properties/`)

See `docs/aerodynamic-breakup/README.md` for detailed documentation.

## Usage

### 1. Set up environment

```bash
export BASILISK=/path/to/basilisk/src
cd cases/fig10_axisymmetric_drop_vaporization
```

### 2. Compile the solver

```bash
cd scripts
./compile.sh
```

This compiles `drop_vaporization_nondim.c` and creates the executable in `build/`.

### 3. Run a single test case

```bash
# Run with default Re=100, We=1.5
./run_single_test.sh 100 1.5

# Run with Re=50
./run_single_test.sh 50 1.5
```

Results are saved in `results/Re_100.00_We_1.50/`.

### 4. Run parameter sweep

```bash
# Run all 25 cases (Re from 22 to 200)
./run_parameter_sweep.sh

# Custom sweep
./run_parameter_sweep.sh 50 150 10  # 10 cases from Re=50 to Re=150
```

Results are saved in `results/Re_XX.XX_We_1.50/` for each case.

## Command-Line Arguments

The solver accepts the following arguments:

```bash
./drop_vaporization_nondim [OPTIONS]

OPTIONS:
  RXXX    - Reynolds number (e.g., R100 for Re=100)
  WXXX    - Weber number (e.g., W1.5 for We=1.5)
  SXXX    - Stefan number (e.g., S0.1 for St=0.1)
  PXXX    - Prandtl number (e.g., P1.0 for Pr=1.0)
  NXXX    - Minimum refinement level (e.g., N6)
  XXXX    - Maximum refinement level (e.g., X12)
  TsXXX   - Output timestep (e.g., Ts0.05)
  TeXXX   - End time (e.g., Te6.4)
```

**Examples:**
```bash
# Re=100, We=1.5 (default parameters)
./drop_vaporization_nondim R100 W1.5

# Re=50, We=1.5, max level 10
./drop_vaporization_nondim R50 W1.5 X10

# Re=150, We=1.5, St=0.2, Pr=0.7
./drop_vaporization_nondim R150 W1.5 S0.2 P0.7
```

## Output Files

Each simulation produces:

### 1. Volume History (`volume_history.txt`)
```
# Columns: t+ | t* | Volume | V/V0
# t+ = solver time (nondimensional)
# t* = paper time = t+ * sqrt(ρ_l/ρ_g)
```

### 2. Snapshots (`intermediate/snapshot-*.dump`)
Full field dumps for restart and visualization

### 3. Duration File (`duration-CPU00.plt`)
Timing information and performance metrics

### 4. Parameters File (`parameters.txt`)
Complete record of all simulation parameters

## Validation Metrics

Compare against Boyd & Ling 2023 Figure 10:

1. **Drop volume evolution** - V/V₀ vs t*
2. **Drop shape** - Interface contours
3. **Temperature field** - Thermal boundary layer
4. **Vaporization rate** - dV/dt vs Re
5. **Nusselt number** - Heat transfer correlation

## Nondimensional Formulation Details

### Time Scales

Two time scales are used:

1. **Solver time** (t⁺):
   ```
   t⁺ = t U∞ / D₀
   ```
   This is the time variable in the code.

2. **Paper time** (t*):
   ```
   t* = t⁺ √(ρ_l / ρ_g)
   ```
   This is the time scale used in Boyd & Ling 2023.

Conversion: `t* = t⁺ * 40.04` (for ρ_l/ρ_g = 1605)

### Derived Properties

From Reynolds number Re:
```c
μ_g* = η / Re                    // Gas viscosity
μ_l* = μ_g* / μ_ratio           // Liquid viscosity
```

From Weber number We:
```c
σ* = η / We = 0.000415          // Surface tension
```

Thermal diffusivities:
```c
α_g* = 1 / (Re Pr)              // Gas thermal diffusivity
α_l* = α_g* * k_ratio / (η * cp_ratio)  // Liquid thermal diffusivity
```

### Stefan Condition Implementation

Mass flux at interface:
```c
ṁ* = -(1/(St Re Pr)) |∇T*|_interface
```

Where:
- `|∇T*|` is computed from finite differences
- Negative sign: heat flows into liquid (evaporation)
- Positive ṁ* means mass from liquid to gas

VOF source term:
```c
∂f/∂t = ... - ṁ*/ρ_l*
```

Energy sink:
```c
∂T/∂t = ... - (St ṁ*)/(ρ* c_p*)
```

## Comparison with ImpactForce-main

This solver extends the ImpactForce-main structure with:

### Added Features:
1. **Temperature field** as tracer with phase-dependent diffusivity
2. **Evaporation model** via Stefan condition
3. **Energy equation** with latent heat sink
4. **Thermal boundary conditions** at inflow/outflow

### Retained Structure:
1. **Constants header** pattern from `constants.h`
2. **CFD values structure** for parameter management
3. **Command-line parsing** for Re, We, etc.
4. **Timing diagnostics** and performance tracking
5. **Adaptive refinement** on multiple fields
6. **Output structure** with snapshots and logs

## Parameter Sweep

Default sweep configuration (reproduces Figure 10):

```yaml
Reynolds: [22, 200]  # 25 linearly spaced values
Weber: 1.5           # Fixed
Stefan: 0.1          # Fixed
Prandtl: 1.0         # Fixed
```

Reynolds numbers tested:
```
22.00, 29.58, 37.17, 44.75, 52.33, 59.92, 67.50, 75.08, 82.67, 90.25,
97.83, 105.42, 113.00, 120.58, 128.17, 135.75, 143.33, 150.92, 158.50,
166.08, 173.67, 181.25, 188.83, 196.42, 200.00
```

## Troubleshooting

### Compilation Issues

**Problem**: `qcc: command not found`
**Solution**:
```bash
export BASILISK=/path/to/basilisk/src
export PATH=$BASILISK:$PATH
```

**Problem**: `constants_vaporization.h: No such file`
**Solution**: Ensure you're in the build directory or use the compile.sh script

### Runtime Issues

**Problem**: Simulation crashes or gives NaN
**Solution**:
- Reduce CFL number (in constants_vaporization.h)
- Decrease max refinement level (use X10 instead of X12)
- Check initial temperature profile is smooth

**Problem**: Very slow convergence
**Solution**:
- Increase TOLERANCE (default: 1e-6)
- Reduce maximum level temporarily
- Check if density ratio is causing stiffness

### Performance Issues

**Problem**: Simulation takes too long
**Solution**:
- Reduce max level (X10 instead of X12)
- Increase output timestep
- Use coarser Reynolds sweep (10 cases instead of 25)

## Testing and Validation

### Quick Test (verify compilation and basic run)

```bash
cd scripts
./compile.sh
./run_single_test.sh 100 1.5

# Should complete in ~10 minutes and produce:
# - volume_history.txt
# - intermediate/snapshot-*.dump files
# - parameters.txt
```

### Full Validation

```bash
# Run parameter sweep
./run_parameter_sweep.sh

# Should produce 25 successful cases
# Check summary:
cat ../results/sweep_summary.txt

# Verify volume decreases (evaporation):
grep "V/V0" ../results/Re_100.00_We_1.50/volume_history.txt
```

## References

### Primary References
- **Boyd & Ling (2023)** - Computers and Fluids
  - Figure 10: Axisymmetric drop vaporization
  - Equations 51-53: Stefan condition and temperature profile

- **A consistent volume-of-fluid approach for direct numerical simulation of the aerodynamic breakup of a vaporizing drop**
  - Aerodynamic breakup with vaporization methodology
  - Advanced VOF techniques for phase change
  - See `docs/aerodynamic-breakup/` for full paper

### Code References
- **ImpactForce-main/Bdropimpact.c** - Base solver structure
- **ImpactForce-main/constants.h** - Configuration pattern
- **test_drop_vap_nondim.c** - Initial nondimensional formulation

### Basilisk Documentation
- http://basilisk.fr - Main documentation
- http://basilisk.fr/src/two-phase.h - Two-phase flow module
- http://basilisk.fr/src/axi.h - Axisymmetric geometry

## Contact and Support

For issues with this test suite:
1. Check `IMPLEMENTATION_NOTES.md` for known issues
2. Review Basilisk documentation
3. Examine example cases in `$BASILISK/examples/`

For general Basilisk questions:
- Visit: http://basilisk.fr
- Mailing list: basilisk@basilisk.fr

## Version History

- **2025-11-18**: Enhanced test suite with aerodynamic breakup cases
  - Added aerodynamic breakup vaporization test cases
  - Integrated Stefan problem, bubble growth, film boiling, and sucking problem
  - Reorganized structure with comprehensive header library
  - Added documentation for aerodynamic breakup tests

- **2025-11-18**: Initial complete test suite
  - Full nondimensional solver
  - YAML equation specification
  - ImpactForce-main based structure
  - Parameter sweep scripts

## License

This test suite is part of the basilisk-local project.
See main repository LICENSE for details.
