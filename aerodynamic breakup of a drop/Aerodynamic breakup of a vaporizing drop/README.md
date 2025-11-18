# Aerodynamic Breakup of a Vaporizing Drop

This repository contains direct numerical simulation (DNS) code for studying the aerodynamic breakup of vaporizing drops using a consistent volume-of-fluid (VOF) approach. The implementation is based on the methodology described in Boyd and Ling (2023).

## Overview

This project implements a comprehensive numerical framework for simulating phase-change phenomena and aerodynamic breakup of liquid drops. The code uses the Basilisk solver with adaptive mesh refinement to capture the complex physics of vaporization and drop deformation.

## Reference

The methods and validation cases in this repository are based on:

**Boyd, B., & Ling, Y. (2023).** *A consistent volume-of-fluid approach for direct numerical simulation of the aerodynamic breakup of a vaporizing drop.* Computers & Fluids, 105807.
- DOI: [10.1016/j.compfluid.2023.105807](https://doi.org/10.1016/j.compfluid.2023.105807)
- URL: https://www.sciencedirect.com/science/article/pii/S0045793023000324

The full paper is included in this directory as a PDF.

## Directory Structure

```
.
├── A consistent volume-of-fluid approach for direct numerical simulation of the aerodynamic breakup of a vaporizing drop.pdf
├── src/                          # Source code
│   ├── 01_vaporization/         # Core vaporization model implementation
│   ├── properties/              # Fluid properties (water, vapor, film boiling)
│   ├── theory_evap/             # Analytical solutions for validation
│   ├── LS_funcs/                # Level-set functions
│   ├── common_functions/        # Utility functions (bisection, secant, trapezoid)
│   ├── post_processing/         # Visualization tools (iso3D, movie_maker)
│   └── outputs/                 # Output utilities (logging, progress tracking)
├── run/                         # Test cases and simulation drivers
│   ├── bubble_growth.c          # Section 4.3: Bubble growth in superheat liquid
│   ├── film_boiling.c           # Section 4.4: 2D film boiling problem
│   ├── stefan_problem.c         # Section 4.1: Stefan flow problem
│   ├── sucking_problem.c        # Section 4.2: Sucking problem
│   └── Makefile.txt             # Build instructions
└── images/                      # Domain setup diagrams and sample results
    ├── bubble_setup.png
    ├── film_domain.png
    ├── stef.png
    ├── stef_plus_suck.png
    ├── suck.png
    └── vaporizing_breaking_drop.png
```

## Test Cases

The repository includes four validation test cases from Boyd et al. (2023):

### 1. Stefan Flow Problem (Section 4.1)
**File:** `run/stefan_problem.c`

Simulates a 1D vapor layer growing from a hot wall. This canonical problem tests the phase-change model against an analytical solution.

- Initial vapor layer thickness: 0.1 mm
- Domain size: 1.0 mm
- Fluid: Water/vapor at saturation conditions
- Wall temperature: T_sat + 10 K

### 2. Sucking Problem (Section 4.2)
**File:** `run/sucking_problem.c`

Models the "sucking" flow induced by evaporation at a liquid-vapor interface, where the interface recedes and draws liquid toward it.

- Initial vapor layer thickness: 0.02 m
- Domain size: 1.0 m
- Superheat: 2 K above saturation
- Includes analytical solution comparison

### 3. Bubble Growth (Section 4.3)
**File:** `run/bubble_growth.c`

Simulates bubble growth in a superheated liquid using 3D octree mesh or 2D axisymmetric configuration.

- Initial bubble radius: 0.12 m
- Domain: 5× initial radius
- Liquid superheat: 2 K above saturation
- Includes bubble volume tracking and comparison with analytical solution

### 4. Film Boiling (Section 4.4)
**File:** `run/film_boiling.c`

2D film boiling simulation with Rayleigh-Taylor instability at the vapor-liquid interface.

- Domain determined by RT wavelength
- Gravity: 9.81 m/s²
- Wall superheat: 5 K above saturation
- Demonstrates vapor bubble formation and release

## Key Features

- **Volume-of-Fluid (VOF) method** for interface tracking
- **Adaptive mesh refinement (AMR)** for computational efficiency
- **Phase-change modeling** with mass and energy transfer
- **Two-phase flow** with temperature-dependent properties
- **Analytical solutions** for validation
- **3D visualization** support with movie generation
- **MPI parallelization** support for large-scale simulations

## Dependencies

This code requires the [Basilisk solver](http://basilisk.fr/):
- Basilisk C framework
- C compiler (GCC recommended)
- Optional: MPI for parallel runs
- Optional: gnuplot for plotting results

## Building and Running

### Compilation

Each test case can be compiled using the Basilisk `qcc` compiler:

```bash
cd run/
qcc -O2 -Wall bubble_growth.c -o bubble_growth -lm
qcc -O2 -Wall film_boiling.c -o film_boiling -lm
qcc -O2 -Wall stefan_problem.c -o stefan_problem -lm
qcc -O2 -Wall sucking_problem.c -o sucking_problem -lm
```

For parallel execution with MPI:
```bash
CC99='mpicc -std=c99' qcc -Wall -O2 -D_MPI=1 bubble_growth.c -o bubble_growth -lm
```

### Execution

Run the compiled executable:
```bash
./bubble_growth
```

For MPI runs:
```bash
mpirun -np 4 ./bubble_growth
```

## Output and Visualization

Each simulation generates:
- **Data files** (`.dat`) with quantitative results
- **Movie files** (`.mp4`) showing temperature and interface evolution
- **Comparison plots** with analytical solutions (via gnuplot)
- **Progress output** to monitor simulation status

Results are compared against analytical solutions where available, allowing for quantitative validation of the numerical method.

## Mesh Refinement

The code uses adaptive mesh refinement based on:
- Volume fraction gradients (`femax`)
- Temperature gradients (`Temax`)
- Velocity gradients (`uemax`)

Refinement levels can be adjusted via `MIN_LEVEL`, `LEVEL`, and `MAX_LEVEL` parameters in each test case.

## Physical Properties

Fluid properties are defined in `src/properties/`:
- `water_vapor.h` - Water/vapor properties at saturation
- `liq_gas.h` - Generic liquid/gas properties
- `film_boiling_case.h` - Properties for film boiling simulations

## Citation

If you use this code in your research, please cite:

```bibtex
@article{boyd_consistent_2023,
  title = {A consistent volume-of-fluid approach for direct numerical simulation of the aerodynamic breakup of a vaporizing drop},
  journal = {Computers \& Fluids},
  author = {Boyd, Bradley and Ling, Yue},
  year = {2023},
  pages = {105807},
  issn = {0045-7930},
  doi = {10.1016/j.compfluid.2023.105807},
  keywords = {DNS, Drop breakup, Vaporization, Volume-of-fluid method}
}
```

## Contact

For questions or issues related to this implementation, please refer to the original publication or contact the authors.

## License

Copyright: All rights reserved (as specified in the source files)
