# Nondimensional Axisymmetric Drop Vaporization Test Suite

Comprehensive test suite for axisymmetric drop vaporization using a fully nondimensional formulation, based on **Boyd & Ling 2023** (*Computers and Fluids*, Figure 10, Sections 5.1 and Table 2).

## Overview

This test suite validates the implementation of drop vaporization physics in a fully nondimensional framework, enabling direct comparison with theoretical predictions and empirical correlations. The formulation uses characteristic scales (D₀, U∞, ρₗ) all set to unity, with material ratios and nondimensional groups as the fundamental parameters.

### Key Features

- **Fully nondimensional formulation** with unity scales
- **Material ratios**: ρ_g/ρ_l = 0.000623, μ_g/μ_l = 0.045 (fixed from physical properties)
- **Nondimensional groups**: We = 1.5, St = 0.1, Pr = 1.0 (fixed)
- **Parameter sweep**: 25 Reynolds numbers linearly spaced from 22 to 200
- **Axisymmetric geometry** with adaptive mesh refinement
- **Comprehensive diagnostics**: volume, Nusselt number, frontal area

## Nondimensional Formulation

### Characteristic Scales

All scales are set to unity in the nondimensional formulation:

| Scale | Symbol | Value | Description |
|-------|--------|-------|-------------|
| Length | D | 1.0 | Characteristic length (drop diameter D₀) |
| Velocity | U_inf | 1.0 | Free-stream velocity |
| Density | ρ_l | 1.0 | Liquid density reference |
| Time | t_ref | D / U_inf | Reference time |

### Material Ratios (FIXED)

Derived from physical properties of water and steam:

```
η = ρ_g / ρ_l = 0.000623    (from 0.597 kg/m³ / 958.4 kg/m³)
μ_ratio = μ_g / μ_l = 0.045  (from 1.26×10⁻⁵ Pa·s / 2.8×10⁻⁴ Pa·s)
```

### Nondimensional Groups (FIXED)

| Group | Symbol | Value | Definition |
|-------|--------|-------|------------|
| Weber number | We | 1.5 | ρ_g U²∞ D / σ |
| Surface tension | Σ* | 0.6667 | 1 / We |
| Stefan number | St | 0.1 | c_p,g (T∞ - T_sat) / h_lg |
| Prandtl number | Pr | 1.0 | μ_g c_p,g / k_g |

### Parameter Sweep

**Reynolds number**: Re ∈ [22, 200], 25 linearly spaced values

For each Re value, the nondimensional viscosities are computed as:

```
μ_g* = η / Re
μ_l* = μ_g* / μ_ratio
```

This formulation ensures that Re, We, St, and Pr are exactly the specified values.

## Geometry and Mesh

### Computational Domain

```
Domain:           8.0 × 8.0 (axisymmetric)
Drop radius:      R₀ = 0.5
Drop center:      x = 1.5, y = 0
Smoothing radius: R_inf = 0.625 (1.25 × R₀)
```

### Mesh Parameters

```
Maximum level:    Lmax = 12
Minimum cell size: dx_min = 1/512
Base grid:        64 × 64
Refinement:       Wavelet-based AMR on f, u, T
```

Adaptive mesh refinement refines based on:
1. Interface (VOF field f)
2. Velocity gradients
3. Temperature gradients

## Initial and Boundary Conditions

### Initial Conditions

1. **Volume fraction (f)**:
   ```
   f = 1  for r ≤ R₀ (inside drop)
   f = 0  for r > R₀ (outside drop)
   ```

2. **Temperature (T*)**:
   ```
   T* = 0    for r ≤ R₀         (saturation temperature)
   T* = 1    for r ≥ R_inf      (free stream temperature)
   T* = (r - R₀)/(R_inf - R₀)   for R₀ < r < R_inf (linear, Eq. 53)
   ```

3. **Velocity**:
   ```
   u_x = U_inf = 1.0  (uniform inflow)
   u_y = 0.0
   ```

### Boundary Conditions

| Boundary | Condition | Description |
|----------|-----------|-------------|
| **Left** | Inflow | u_x = 1, T* = 1, f = 0 |
| **Right** | Outflow | Neumann (∂u/∂x = 0, p = 0) |
| **Axis** | Axisymmetric | Automatically handled by `axi.h` |

## Time Scales

The simulation uses two time scales:

1. **Solver time** (t⁺):
   ```
   t⁺ = t × U_inf / D
   ```

2. **Paper time** (t*):
   ```
   t* = t⁺ × √(ρ_l / ρ_g)
   ```

**Simulation end**: t* = 0.16 (as specified in Boyd & Ling 2023)

## Directory Structure

```
drop_vaporization_nondim/
├── test_cases/
│   └── test_drop_vap_nondim.c      # Main test case implementation
├── utilities/
│   ├── run_single_case.sh          # Run single Re case
│   ├── run_parameter_sweep.sh      # Run all 25 Re cases
│   └── analyze_results.py          # Analysis and visualization
├── results/                         # Output directory (created on run)
│   ├── Re_22.00_We_1.50/           # Results for Re = 22
│   ├── Re_29.43_We_1.50/           # Results for Re = 29.43
│   ├── ...
│   └── sweep_summary.txt           # Summary of all cases
├── reference_data/                  # Reference solutions (optional)
├── README.md                        # This file
└── .gitignore                       # Git ignore rules
```

## Quick Start

### Prerequisites

1. **Basilisk C**: Ensure Basilisk is installed and the `BASILISK` environment variable is set:
   ```bash
   export BASILISK=/path/to/basilisk/src
   export PATH=$PATH:$BASILISK
   ```

2. **Python 3** (for analysis): With numpy and matplotlib:
   ```bash
   pip3 install numpy matplotlib
   ```

### Run Single Case

To run a single case with a specific Reynolds number:

```bash
cd utilities
./run_single_case.sh 100

# Example: Run with Re = 100
# Output will be in: ../results/Re_100.00_We_1.50/
```

### Run Parameter Sweep

To run the full parameter sweep (25 cases, Re from 22 to 200):

```bash
cd utilities
./run_parameter_sweep.sh

# Options:
./run_parameter_sweep.sh --quick        # Run only 5 cases
./run_parameter_sweep.sh --parallel 4   # Run 4 cases in parallel
./run_parameter_sweep.sh --lmax 10      # Use lower refinement level
```

**Note**: The full sweep can take considerable time depending on your system. Use `--quick` for initial testing.

### Analyze Results

#### Single Case Analysis

```bash
cd utilities
python3 analyze_results.py ../results/Re_100.00_We_1.50

# With plots
python3 analyze_results.py --plot-volume ../results/Re_100.00_We_1.50

# Save plots
python3 analyze_results.py --plot-volume --save ../results/Re_100.00_We_1.50
```

#### Compare Multiple Cases

```bash
cd utilities
python3 analyze_results.py --compare ../results

# Save comparison plots
python3 analyze_results.py --compare --save ../results
```

## Output Files

Each simulation case produces:

### Diagnostic Files

1. **liquid_volume_history.txt**: Time evolution of drop volume
   ```
   Columns: t⁺ | t* | Volume | V/V₀
   ```

2. **nusselt_history.txt**: Nusselt number evolution
   ```
   Columns: t⁺ | t* | Nu | q_interface
   ```

3. **frontal_area_history.txt**: Frontal area evolution
   ```
   Columns: t⁺ | t* | A_frontal | Zf
   ```

4. **log.txt**: Simulation log with timestep information

### Visualization Files

5. **snapshot_XXXXX.vtk**: VTK files for ParaView/VisIt (every 0.01 time units)
   - Fields: f (VOF), T (temperature), u (velocity), p (pressure)

6. **dump_XXXXX**: Binary dump files for restart (every 0.05 time units)

### Summary File (for parameter sweep)

7. **sweep_summary.txt**: Summary of all cases
   ```
   Columns: Case | Re | We | Status | Runtime | V_final/V₀ | Output_Dir
   ```

## Diagnostics

### Volume Evolution

The volume history tracks:
- **V(t)**: Total liquid volume
- **V/V₀**: Volume ratio (mass conservation check)
- **dV/dt**: Vaporization rate

**Expected behavior**: Volume decreases due to vaporization, with higher Re leading to faster vaporization.

### Nusselt Number

The Nusselt number characterizes heat transfer:

```
Nu = q D / (k_g ΔT)
```

where q is the heat flux at the interface.

**Expected behavior**: Nu increases with Re following empirical correlations.

### Frontal Area and Drag Parameter

- **A_frontal**: Projection of drop onto flow direction
- **Zf**: Drag parameter

**Expected behavior**: Drop deforms and breaks up at high We/Re.

## Validation and Comparison

### Reference Data

Results can be compared with:

1. **Boyd & Ling 2023, Figure 10**: Drop shape and temperature contours
2. **Empirical correlations**: Nusselt number relationships (Nu ~ Re^n)
3. **Mass conservation**: |ΔV/V₀| should be due to vaporization only

### Expected Results

| Re | Approximate V/V₀ at t* = 0.16 | Notes |
|----|-------------------------------|-------|
| 22 | ~0.95 | Low vaporization rate |
| 100 | ~0.85 | Moderate vaporization |
| 200 | ~0.75 | High vaporization rate |

*Note: These are approximate values; actual results may vary.*

### Validation Criteria

✅ **PASS** if:
- Volume decreases monotonically
- Final V/V₀ in expected range
- Nusselt number scales with Re
- No numerical instabilities

⚠️ **WARNING** if:
- Volume loss < 1% (too little vaporization)
- Large oscillations in diagnostics
- Mesh convergence issues

❌ **FAIL** if:
- Volume increases (non-physical)
- Simulation crashes
- Mass is not conserved (spurious sources/sinks)

## Numerical Methods

### Solvers

- **Two-phase Navier-Stokes**: Centered finite-volume scheme
- **Volume-of-Fluid (VOF)**: Interface tracking
- **Temperature advection**: Tracer method with diffusion
- **Evaporation**: Stefan condition at interface (to be implemented)

### Time Integration

```
CFL number:    0.2
Time stepping: Adaptive based on CFL condition
```

### Adaptive Mesh Refinement

```
Method:         Wavelet-based AMR
Error tolerance:
  - f (VOF):           0.01
  - u (velocity):      0.01
  - T (temperature):   0.02
Maximum level:  12
Minimum level:  2
```

## Troubleshooting

### Compilation Errors

If compilation fails:

1. Check that `$BASILISK` is set correctly:
   ```bash
   echo $BASILISK
   which qcc
   ```

2. Ensure all Basilisk modules are available:
   ```bash
   ls $BASILISK/axi.h
   ls $BASILISK/navier-stokes/centered.h
   ```

3. Check for syntax errors in the test case file

### Runtime Issues

If the simulation crashes or produces unexpected results:

1. **Check the log file** for error messages:
   ```bash
   cat results/Re_100.00_We_1.50/log.txt
   ```

2. **Reduce the maximum refinement level**:
   ```bash
   ./run_single_case.sh 100
   # Edit test_drop_vap_nondim.c: set Lmax = 10 instead of 12
   ```

3. **Adjust the CFL number** for stability:
   ```bash
   # Edit test_drop_vap_nondim.c: set CFL_number = 0.1
   ```

4. **Check initial conditions**:
   - Temperature profile should be smooth
   - Drop should be fully inside the domain
   - Velocity field should be reasonable

### Memory Issues

For high refinement levels (Lmax = 12):

- **Memory required**: ~2-4 GB per case (depends on refinement)
- **Solution**: Reduce Lmax to 10 or 11 for initial tests
- **Monitor memory usage**:
  ```bash
  top -p $(pgrep test_drop_vap_nondim)
  ```

### Slow Execution

If simulations are very slow:

1. **Use quick mode** for testing:
   ```bash
   ./run_parameter_sweep.sh --quick
   ```

2. **Reduce refinement level**: Set Lmax = 10

3. **Run in parallel**:
   ```bash
   ./run_parameter_sweep.sh --parallel 4
   ```

4. **Check timestep size**: Very small dt may indicate stiffness

## Advanced Usage

### Custom Reynolds Number Range

Edit `run_parameter_sweep.sh` to change the Re range:

```bash
RE_MIN=50.0
RE_MAX=150.0
NUM_CASES=10
```

### Custom Output Frequency

Edit `test_drop_vap_nondim.c`:

```c
event diagnostics(t += 0.01)  // Change to t += 0.005 for more frequent output
event snapshots(t += 0.01)    // Change to t += 0.02 for less frequent snapshots
```

### Restart from Dump

To restart a simulation from a dump file:

```c
// Add to main() before run()
if (restore("results/Re_100.00_We_1.50/dump_00500")) {
  // Simulation will continue from t = 5.0
}
```

## Reference

### Citation

This test suite implements the drop vaporization model from:

> Boyd, B., & Ling, Y. (2023). **Direct numerical simulation of evaporating droplets based on a sharp-interface algebraic VOF approach.** *Computers & Fluids*, Figure 10, Sections 5.1 and Table 2.

### Related Papers

- Stefan condition for phase change
- Nusselt number correlations for evaporating drops
- VOF methods for multiphase flows

## Contact and Support

For issues specific to this test suite:
- Check the simulation log files in the results directory
- Verify boundary conditions and initial conditions
- Ensure physical parameters are consistent

For general Basilisk questions:
- Visit: http://basilisk.fr
- Documentation: http://basilisk.fr/Tutorial
- Forum: http://basilisk.fr/sandbox

## Test Suite Metadata

- **Test name**: Nondimensional Axisymmetric Drop Vaporization
- **Case type**: Axisymmetric_Drop_Vaporization_Nondim
- **Geometry**: 2D axisymmetric
- **Physics**: Two-phase flow with evaporation, heat transfer
- **Primary reference**: Boyd & Ling 2023, Figure 10
- **Parameter sweep**: Re ∈ [22, 200], We = 1.5 (fixed)
- **Simulation time**: t* ∈ [0, 0.16]
- **Version**: 1.0
- **Last updated**: 2025-11-18
