# Figure 10: Axisymmetric Drop Vaporization

This case reproduces **Figure 10** from the paper, which simulates an axisymmetric vaporizing water drop in a hot steam environment. The simulation captures the complex interaction between two-phase flow, heat transfer, and phase change at the liquid-gas interface.

## Physical Problem

### Geometry
- **Domain**: 2D axisymmetric, 8D₀ × 8D₀ (square domain)
- **Drop**: Spherical water drop of diameter D₀, centered at (1.5D₀, 0)
- **Reference length**: D₀ (drop diameter)

### Physics
- **Liquid phase**: Water at saturation temperature (Tₛₐₜ = 373.15 K)
- **Gas phase**: Steam at free-stream temperature (T∞ = 600 K)
- **Surface tension**: σ = 0.0728 N/m
- **Phase change**: Vaporization at the interface with latent heat h_lg = 2.26×10⁶ J/kg

### Thermophysical Properties

| Property | Liquid (Water) | Gas (Steam) | Units |
|----------|----------------|-------------|-------|
| Density (ρ) | 958.4 | 0.597 | kg/m³ |
| Dynamic viscosity (μ) | 2.8×10⁻⁴ | 1.26×10⁻⁵ | Pa·s |
| Thermal conductivity (k) | 0.679 | 0.025 | W/(m·K) |
| Specific heat (cp) | 4216 | 2030 | J/(kg·K) |

### Dimensionless Numbers

- **Weber number** (We): Fixed at 1.5
- **Reynolds number** (Re): Swept from 22 to 200 (25 cases)
- **Stefan number** (St): 0.1
- **Prandtl number** (Pr): 1.0

The dimensionless numbers are defined as:
- We = ρ_g U∞² D₀ / σ
- Re = ρ_g U∞ D₀ / μ_g
- St = cp_g (T∞ - Tₛₐₜ) / h_lg
- Pr = μ_g cp_g / k_g

### Initial Conditions

1. **Velocity field**: Uniform inflow from the left with velocity U∞
2. **Volume fraction**: Spherical drop of radius R₀ = D₀/2
3. **Temperature field** (Eq. 53):
   ```
   T(r ≤ R₀) = Tₛₐₜ  (inside drop)
   T(r ≥ R_inf) = T∞  (far field, R_inf = 1.25 R₀)
   T(R₀ < r < R_inf) = linear interpolation
   ```

### Boundary Conditions

- **Left**: Inflow with velocity U∞ and temperature T∞
- **Right**: Outflow (Neumann conditions)
- **Top/Bottom**: Axisymmetric boundaries

### Adaptive Mesh Refinement

- **Maximum level**: 12 (minimum cell size = D₀/512)
- **Refinement criteria**: Interface (VOF), velocity gradients, temperature gradients
- **Refinement band**: 5 cells on each side of the interface

## File Structure

```
fig10_axisymmetric_drop_vaporization/
├── src/
│   └── fig10_drop_vaporization.c    # Main simulation code
├── scripts/
│   ├── run_single_case.sh           # Run a single Re case
│   ├── run_parameter_sweep.sh       # Run all 25 Re cases
│   └── visualize_results.py         # Visualization script
├── results/                         # Output directory (created on run)
│   ├── Re_22.00_We_1.50/           # Results for each case
│   ├── Re_29.58_We_1.50/
│   └── ...
├── build/                           # Build directory (created on compile)
└── README.md                        # This file
```

## Prerequisites

1. **Basilisk C**: Ensure Basilisk is installed and the `BASILISK` environment variable is set:
   ```bash
   export BASILISK=/path/to/basilisk/src
   ```

2. **Python 3** (for visualization): With numpy and matplotlib:
   ```bash
   pip install numpy matplotlib
   ```

## Running the Simulation

### Single Case

To run a single case with specific Reynolds and Weber numbers:

```bash
cd scripts
./run_single_case.sh [REYNOLDS] [WEBER]

# Example: Run with Re = 100, We = 1.5
./run_single_case.sh 100 1.5
```

The results will be saved in `results/Re_100.00_We_1.50/`.

### Parameter Sweep

To run the full parameter sweep (25 cases, Re from 22 to 200):

```bash
cd scripts
./run_parameter_sweep.sh
```

This will:
1. Compile the code
2. Run 25 simulations with different Reynolds numbers
3. Save results for each case in separate directories
4. Create a summary file `results/sweep_summary.txt`

**Note**: The full sweep can take considerable time depending on your system.

## Output Files

Each simulation case produces:

1. **volume_history.txt**: Time evolution of drop volume
   - Columns: t [s], t* [-], Volume [D₀³], V/V₀ [-]

2. **snapshot_*.vtk**: VTK files for visualization (every 50 timesteps)
   - Can be opened with ParaView or VisIt

3. **fields_***: Binary dump files containing all field data
   - Can be reloaded in Basilisk for post-processing

4. **log.txt**: Simulation log with diagnostics

## Visualization

### Single Case Analysis

To visualize results from a single case:

```bash
cd scripts
./visualize_results.py ../results/Re_100.00_We_1.50
```

This creates plots showing:
- Drop volume evolution over time
- Vaporization rate

### Comparison Across Reynolds Numbers

To compare results across all Reynolds numbers:

```bash
cd scripts
./visualize_results.py ../results --compare --We 1.5
```

This creates a comparison plot with:
- Volume evolution for all Re values
- Vaporization rates for all Re values

## Key Results and Diagnostics

### Volume Evolution

The volume history shows how the drop shrinks due to vaporization. The dimensionless time is defined as:

**t\* = t · U∞/D₀ · √(ρₗ/ρg)**

### Vaporization Rate

The mass flux at the interface is computed from the temperature gradient (Eq. 51-53):

**ṁ = k_g |∇T|_interface / h_lg**

### Nusselt Number

The Nusselt number characterizes heat transfer:

**Nu = q D₀ / (k_g ΔT)**

where q is the heat flux at the interface.

## Numerical Methods

### Solvers
- **Two-phase Navier-Stokes**: Centered finite-volume scheme
- **Volume-of-Fluid (VOF)**: Interface tracking
- **Temperature advection**: Tracer method with diffusion
- **Evaporation**: Stefan condition at interface

### Time Integration
- **CFL**: 0.2
- **Time stepping**: Adaptive based on CFL condition

### Adaptive Mesh Refinement
- **Wavelet-based AMR**: Refines based on gradients in f, u, and T
- **Maximum level**: 12 (4096 cells per diameter)
- **Minimum level**: 2

## Validation and Comparison

The results can be compared with:

1. **Figure 10 in the paper**: Drop shape and temperature contours
2. **Empirical correlations**: Nusselt number relationships
3. **Film temperature model**: Using Tf = (T∞ + Tₛₐₜ)/2

## Troubleshooting

### Compilation Errors

If compilation fails:
1. Check that `$BASILISK` is set correctly
2. Ensure all Basilisk modules are available
3. Check for syntax errors in the source file

### Runtime Issues

If the simulation crashes or produces unexpected results:
1. Check the log file for error messages
2. Reduce the maximum refinement level (Lmax) to save memory
3. Adjust the CFL number for stability
4. Check initial conditions (especially temperature profile)

### Memory Issues

For high refinement levels:
- Maximum level 12 can require significant memory
- Consider reducing Lmax to 10 or 11 for initial tests
- Monitor memory usage during the run

## References

The simulation implements the evaporation model described in Equations (51)-(53) of the paper, using:
- Stefan number formulation for phase change
- Temperature-dependent interface conditions
- Conservative treatment of latent heat

## Contact and Support

For issues specific to this case:
- Check the simulation log files in the results directory
- Verify boundary conditions and initial conditions
- Ensure physical parameters are consistent

For general Basilisk questions:
- Visit: http://basilisk.fr
- Documentation: http://basilisk.fr/Tutorial

## Case Metadata

- **Case name**: Fig10_Axisymmetric_Drop_Vaporization
- **Geometry**: 2D axisymmetric
- **Physics**: Two-phase flow with evaporation
- **Primary figure**: Figure 10 (drop shape and temperature field)
- **Parameter sweep**: Re ∈ [22, 200], We = 1.5 (fixed)
- **Simulation time**: t* ∈ [0, 0.16]
