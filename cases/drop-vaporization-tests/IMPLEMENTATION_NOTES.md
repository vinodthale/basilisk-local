# Implementation Notes: Figure 10 Axisymmetric Drop Vaporization

## Overview

This directory contains the implementation of the Figure 10 axisymmetric drop vaporization simulation. The case models a water drop vaporizing in a hot steam environment with two-phase flow, heat transfer, and phase change.

## Implementation Status

### ✅ Completed Features

1. **Basic two-phase flow setup**
   - VOF (Volume-of-Fluid) interface tracking
   - Density ratio: ρ_l/ρ_g ≈ 1605
   - Viscosity ratio: μ_l/μ_g ≈ 22
   - Surface tension with Weber number We = 1.5

2. **Geometric setup**
   - 2D Cartesian simulation (axisymmetric version pending)
   - Domain: 8D₀ × 8D₀
   - Drop radius: R₀ = 0.5D₀
   - Drop center: (1.5D₀, 0)

3. **Boundary conditions**
   - Left: inflow with U∞ = 1 (dimensionless)
   - VOF boundary conditions for gas phase

4. **Adaptive mesh refinement**
   - Wavelet-based AMR on VOF field and velocity
   - Maximum level: 8 (can be increased to 10-12)
   - Minimum level: 2

5. **Output and diagnostics**
   - Volume tracking every 10 timesteps
   - Drop volume history

6. **Parameter sweep infrastructure**
   - Script to run multiple Reynolds numbers (22 to 200)
   - 25 cases with fixed Weber number
   - Automated result organization

7. **Visualization tools**
   - Python scripts for analyzing results
   - Volume evolution plots
   - Comparison across Reynolds numbers

### ⏳ Features To Be Added

1. **Axisymmetric geometry**
   - Currently implemented in 2D Cartesian
   - Need to add `axi.h` header
   - Requires careful handling of boundary conditions at r=0

2. **Temperature field**
   - Framework ready using `henry.h` for soluble tracers
   - Initial temperature profile (Eq. 53)
   - Phase-dependent thermal diffusivity
   - Needs to be integrated with evaporation model

3. **Evaporation model**
   - Stefan condition at interface (Eqs. 51-53)
   - Mass flux: ṁ = k_g |∇T|/h_lg
   - Energy balance with latent heat
   - Interface velocity modification due to phase change

4. **Additional diagnostics**
   - Nusselt number calculation
   - Vaporization rate from volume loss
   - Temperature gradient at interface
   - Comparison with empirical correlations

## File Structure

```
fig10_axisymmetric_drop_vaporization/
├── src/
│   ├── drop_vap.c                      # Working 2D implementation (basic)
│   ├── fig10_drop_vaporization.c       # Advanced version (needs debugging)
│   ├── fig10_drop_vaporization_v2.c    # With temperature tracer (to be completed)
│   └── fig10_drop_vaporization_simple.c # Simplified test version
├── scripts/
│   ├── run_single_case.sh              # Run one case
│   ├── run_parameter_sweep.sh          # Run all 25 cases
│   └── visualize_results.py            # Analysis and plotting
├── build/                              # Compilation directory
├── results/                            # Output directory
├── README.md                           # User documentation
└── IMPLEMENTATION_NOTES.md             # This file
```

## Current Working Version: `drop_vap.c`

The `drop_vap.c` file is the currently functional version with:
- ✅ Two-phase VOF
- ✅ Surface tension
- ✅ Adaptive mesh refinement
- ✅ Volume tracking
- ✅ Boundary conditions
- ❌ Temperature field (removed for testing)
- ❌ Evaporation (not yet implemented)
- ❌ Axisymmetric geometry (using 2D Cartesian)

## Key Implementation Lessons

### 1. Variable Naming Conflicts

Basilisk has several built-in variables that cannot be redefined:
- `L0` - domain size
- `dt` - timestep
- Other grid-related variables

**Solution**: Use descriptive names like `drop_diameter`, `domain_size`, etc.

### 2. Include Order Matters

Correct include order for two-phase flow:
```c
#include "grid/multigrid.h"
#include "navier-stokes/centered.h"
#include "two-phase.h"
#include "tension.h"
```

For axisymmetric:
```c
#include "grid/multigrid.h"
#include "axi.h"
#include "navier-stokes/centered.h"
#include "two-phase.h"
#include "tension.h"
```

### 3. Temperature as Soluble Tracer

For phase-dependent properties, use `henry.h`:
```c
#include "henry.h"
scalar T[], * stracers = {T};

// In main():
T.D1 = alpha_liquid;   // Thermal diffusivity in liquid
T.D2 = alpha_gas;      // Thermal diffusivity in gas
T.alpha = 1.0;         // Temperature continuity
```

### 4. Compilation Process

Must compile from the same directory as the source or copy source to build directory:
```bash
export BASILISK=/path/to/basilisk/src
cd build/
cp ../src/drop_vap.c .
$BASILISK/qcc -Wall -O2 -o drop_vap drop_vap.c -lm
```

### 5. Poisson Solver Warnings

Convergence warnings for two-phase flow are common:
```
warning: convergence for p not reached after 100 iterations
```

**Solutions**:
- Increase `TOLERANCE` (default 1e-4)
- Increase `NITERMAX` for Poisson solver
- Reduce CFL number for stability
- Check density/viscosity ratios aren't too extreme

## Next Steps

### Immediate (to complete basic case):

1. **Add axisymmetric geometry**
   - Include `axi.h`
   - Test with simple case first
   - Verify boundary conditions at r=0

2. **Integrate temperature field**
   - Add temperature tracer back to `drop_vap.c`
   - Initialize with Eq. (53) profile
   - Verify diffusion is working

3. **Implement evaporation**
   - Compute temperature gradient at interface
   - Calculate mass flux from Stefan condition
   - Modify interface velocity
   - Add energy sink for latent heat

### Medium-term (for full reproduction):

4. **Validate against paper**
   - Compare drop shapes
   - Verify vaporization rates
   - Check Nusselt number correlations
   - Compare with Figure 10 results

5. **Run parameter sweep**
   - All 25 Reynolds numbers
   - Collect statistics
   - Generate comparison plots

6. **Optimize performance**
   - Profile code
   - Tune AMR parameters
   - Optimize output frequency

## Known Issues

1. **Segmentation fault with complex versions**
   - Some advanced versions cause qcc to segfault
   - Workaround: Build up features gradually
   - May be related to include order or macro definitions

2. **Poisson convergence**
   - High density ratio causes slow convergence
   - May need multigrid parameters tuning

3. **Temperature field integration pending**
   - `henry.h` module tested separately
   - Needs integration with current working version

## References

- Basilisk documentation: http://basilisk.fr
- Examples: `$BASILISK/examples/bubble.c`
- Test cases: `$BASILISK/test/static_bubble.c`
- Two-phase flow: `$BASILISK/src/two-phase.h`
- Soluble tracers: `$BASILISK/src/henry.h`

## Compilation and Testing Commands

```bash
# Set environment
export BASILISK=/home/user/basilisk-local/src

# Compile current working version
cd cases/fig10_axisymmetric_drop_vaporization/build
$BASILISK/qcc -Wall -O2 -o drop_vap ../src/drop_vap.c -lm

# Run single test
./drop_vap 100 1.5

# Run with different parameters
./drop_vap 50 1.5   # Re=50, We=1.5
./drop_vap 150 1.5  # Re=150, We=1.5
```

## Contact and Support

For issues or questions:
1. Check Basilisk documentation
2. Review example cases in `$BASILISK/examples/`
3. Consult the Basilisk mailing list

---

**Last Updated**: 2025-11-18
**Status**: Basic two-phase flow working, temperature and evaporation pending
