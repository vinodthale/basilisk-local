# Basilisk

Basilisk is a powerful computational fluid dynamics (CFD) solver framework for solving partial differential equations using adaptive mesh refinement and multiresolution algorithms on octree/quadtree grids.

## Overview

Basilisk provides a comprehensive suite of numerical solvers for simulating various fluid dynamics phenomena, including:

- **Saint-Venant equations** (shallow water flows)
- **Navier-Stokes equations** (incompressible and compressible flows)
- **Two-phase interfacial flows** (VOF methods)
- **Multilayer flows** with free surfaces
- **Viscoelastic flows**
- **Electrohydrodynamics**
- **All-Mach compressible flows**
- **Conservation laws** and hyperbolic systems

## Key Features

- **Adaptive Mesh Refinement (AMR)**: Automatic grid refinement based on solution features
- **Octree/Quadtree Grids**: Efficient hierarchical data structures for 2D and 3D simulations
- **Multiple Coordinate Systems**: Cartesian, axisymmetric, spherical, and arbitrary orthogonal coordinates
- **Python Interface**: High-level Python bindings for easy simulation setup and control
- **Visualization Tools**: Built-in visualization through Basilisk View (interactive web-based and offline rendering)
- **Parallel Computing**: MPI support for distributed memory parallelism
- **GPU Support**: Acceleration on graphics processing units
- **Custom C Preprocessor**: The `qcc` compiler provides powerful domain-specific extensions

## Installation

### Prerequisites

- C99-compliant compiler (GCC recommended)
- GNU Make or compatible
- Standard UNIX utilities (awk, sed, etc.)

### Optional Dependencies

- `gnuplot` (>= 4.2) - for plotting
- `imagemagick` - for image processing
- `ffmpeg` - for video generation
- `graphviz` - for visualization
- `valgrind` - for debugging
- `gifsicle` - for GIF animation
- `SWIG` and Python development headers - for Python interface
- `gfortran` - for optional Fortran libraries (GOTM, CVMix, PPR)

On Debian/Ubuntu systems:

```bash
sudo apt install build-essential gawk
sudo apt install gnuplot imagemagick ffmpeg graphviz valgrind gifsicle pstoedit
```

### Building from Source

1. Navigate to the source directory:
   ```bash
   cd src
   ```

2. Select the appropriate configuration file:
   ```bash
   ln -s config.gcc config
   ```

   For 32-bit systems:
   ```bash
   ln -s config.gcc.32bits config
   ```

3. Compile:
   ```bash
   make
   ```

4. Add Basilisk to your PATH (optional but recommended):
   ```bash
   export BASILISK=$(pwd)
   export PATH=$PATH:$BASILISK
   echo "export BASILISK=$PWD" >> ~/.bashrc
   echo 'export PATH=$PATH:$BASILISK' >> ~/.bashrc
   ```

### Python Interface Setup

If you want to use Basilisk from Python:

```bash
sudo apt install swig libpython-dev
```

Configure the `MDFLAGS` and `PYTHONINCLUDE` variables in your `config` file, then:

```bash
make stream.py  # Example module
```

## Quick Start

### C Interface

Create a simple file `test.c`:

```c
#include "grid/multigrid.h"
#include "navier-stokes/centered.h"

int main() {
  init_grid (128);
  run();
}
```

Compile and run:

```bash
qcc -o test test.c -lm
./test
```

### Python Interface

See `src/examples/example.py` for a complete example of using Basilisk with Python for simulating 2D turbulence.

## Project Structure

```
basilisk-local/
├── src/              # Main source directory
│   ├── README        # Detailed solver documentation
│   ├── INSTALL       # Installation instructions
│   ├── qcc.c         # Basilisk C preprocessor/compiler
│   ├── *.h           # Solver modules and header files
│   ├── ast/          # Abstract Syntax Tree library
│   ├── grid/         # Grid implementations (multigrid, octree, GPU)
│   ├── navier-stokes/ # Navier-Stokes solvers
│   ├── layered/      # Multilayer flow solvers
│   ├── examples/     # Example programs
│   ├── test/         # Test cases
│   ├── jview/        # Javascript visualization interface
│   ├── darcsit/      # Web interface for code browsing
│   ├── ppr/          # Piecewise Polynomial Reconstruction library
│   ├── gotm/         # General Ocean Turbulence Model interface
│   ├── cvmix/        # Community Ocean Vertical Mixing interface
│   └── wsServer/     # WebSocket server for visualization
```

## Documentation

- **Main Documentation**: See `src/README` for comprehensive solver documentation
- **Tutorial**: Visit the official Basilisk tutorial at http://basilisk.fr/Tutorial
- **Examples**: Browse `src/examples/` and `src/test/` directories
- **Python Interface**: See `src/examples/example.py`

## Features by Category

### Solvers

- Saint-Venant (shallow water)
- Navier-Stokes (multiple formulations: centered, MAC, streamfunction-vorticity)
- All-Mach compressible flows
- Two-phase flows with surface tension
- Hele-Shaw/Darcy flows
- Viscoelastic flows (log-conformation method)
- Electrohydrodynamics
- Multilayer hydrostatic flows

### Numerical Methods

- Volume-Of-Fluid (VOF) interface tracking
- Level-set methods
- Adaptive mesh refinement
- Multigrid solvers
- Poisson/Helmholtz solvers
- Conservative advection schemes
- High-order reconstruction (PPM, PQM)
- WENO-like limiters

### Geometry and Coordinates

- Cartesian (2D/3D)
- Axisymmetric
- Spherical coordinates
- Arbitrary orthogonal coordinates
- Embedded boundaries

## Contributing

Contributions to Basilisk are welcome. The project includes contributions from numerous researchers and developers. See `src/AUTHORS` for a list of contributors.

For detailed contribution guidelines, see `src/Contributing`.

## Testing

Run the test suite:

```bash
cd src
./runtest
```

For automated testing:

```bash
./autotest.sh
```

## License

Basilisk is distributed under the GNU General Public License. See `src/COPYING` for full license text.

Some components have specific licenses:
- PPR library: See `src/ppr/LICENSE.md`
- TinyRenderer: See `src/gl/tinyrenderer/LICENSE.txt`

## Support and Community

- **Official Website**: http://basilisk.fr
- **Source Repository**: This repository contains the full Basilisk source code
- **Issues**: For bugs and feature requests, please use the project's issue tracking system

## Citations

If you use Basilisk in your research, please cite the appropriate papers. See `src/references.bib` for BibTeX entries.

## Additional Resources

- **Debugging**: See `src/README` sections on tracking floating-point exceptions
- **Profiling**: Built-in support for performance profiling (`src/README.trace`)
- **Memory Profiling**: See `src/README.mtrace`
- **Parallel Performance**: Paraver integration (`src/README.paraver`)
- **WASM Support**: WebAssembly compilation (`src/README.wasm`)

---

**Note**: This is a local copy of Basilisk. For the latest version, visit http://basilisk.fr
