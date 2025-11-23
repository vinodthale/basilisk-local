# Basilisk

[![License: GPL](https://img.shields.io/badge/License-GPL-blue.svg)](http://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20WSL-lightgrey.svg)](docs/INSTALLATION.md)

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

## 🚀 Getting Started

### Quick Installation

For detailed installation instructions, system requirements, and troubleshooting, see the **[Installation Guide](docs/INSTALLATION.md)**.

**Quick start for Ubuntu/Debian:**

```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential gawk

# Build Basilisk
cd src
ln -sf config.gcc config
make -j$(nproc)

# Set environment variables
export BASILISK=$PWD
export PATH=$PATH:$BASILISK
echo "export BASILISK=$PWD" >> ~/.bashrc
echo 'export PATH=$PATH:$BASILISK' >> ~/.bashrc
```

### System Requirements

- **OS**: Linux, macOS, or Windows (WSL2)
- **Compiler**: GCC 4.9+ or Clang 3.5+ with C99 support
- **RAM**: 2 GB minimum (4 GB+ recommended)
- **Disk**: 500 MB minimum

See the [Installation Guide](docs/INSTALLATION.md) for complete system requirements.

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

## 📖 Documentation

### Essential Documentation

- **[Installation Guide](docs/INSTALLATION.md)** - Complete setup instructions, system requirements, and troubleshooting
- **[Documentation Hub](docs/README.md)** - Central documentation directory with guides and tutorials
- **[Main Documentation](src/README)** - Comprehensive solver documentation in the source directory
- **[Official Tutorial](http://basilisk.fr/Tutorial)** - Interactive online tutorial

### Technical Guides

- **[Axisymmetric Simulations](docs/guides/BASILISK_AXISYMMETRIC_GUIDE.md)** - Guide for axisymmetric coordinate systems
- **[SHARP VOF Methods](docs/guides/SHARP-VOF-SUMMARY.md)** - Interface tracking with Volume-Of-Fluid methods
- **[Reproduction Guide](docs/guides/REPRODUCTION_GUIDE.md)** - Reproduce simulation results
- **[Code Review Summary](docs/guides/CODE_REVIEW_SUMMARY.md)** - Code quality and review findings

### Examples and Case Studies

- **Examples**: Browse `src/examples/` for sample programs
- **Test Cases**: See `src/test/` for validation cases
- **Case Studies**: Explore `cases/` and `ImpactForce-main/` directories

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

## 💬 Support and Community

- **Official Website**: [basilisk.fr](http://basilisk.fr)
- **Documentation**: [docs/](docs/)
- **Tutorial**: [basilisk.fr/Tutorial](http://basilisk.fr/Tutorial)
- **Issues**: Use GitHub's issue tracker for bug reports and feature requests
- **Source Code**: Full source code available in this repository

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
