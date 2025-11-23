# Basilisk Installation Guide

This guide provides detailed instructions for installing and setting up Basilisk on your system.

## Table of Contents

- [System Requirements](#system-requirements)
- [Prerequisites](#prerequisites)
- [Installation Steps](#installation-steps)
- [Optional Components](#optional-components)
- [Verification](#verification)
- [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements

- **Operating System**: Linux (Ubuntu 18.04+, Debian 9+, or compatible)
- **CPU**: x86_64 or ARM64 architecture
- **RAM**: 2 GB minimum (4 GB+ recommended for 3D simulations)
- **Disk Space**: 500 MB for basic installation, 2 GB+ recommended with examples and documentation
- **Compiler**: GCC 4.9+ or Clang 3.5+ with C99 support

### Recommended Requirements

- **Operating System**: Ubuntu 20.04 LTS or newer
- **CPU**: Multi-core processor (4+ cores for parallel simulations)
- **RAM**: 8 GB or more
- **Disk Space**: 5 GB or more
- **GPU**: NVIDIA GPU with CUDA support (optional, for GPU-accelerated simulations)

### Supported Platforms

- ✅ Linux (primary platform)
- ✅ macOS (with some limitations)
- ✅ Windows (via WSL2 - Windows Subsystem for Linux)
- ⚠️ 32-bit systems (limited support, use `config.gcc.32bits`)

## Prerequisites

### Essential Dependencies

These packages are required for basic Basilisk functionality:

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y build-essential
sudo apt install -y gawk
sudo apt install -y make
```

#### Fedora/RHEL/CentOS

```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install gawk
```

#### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required packages
brew install gcc make gawk
```

### Recommended Dependencies

For visualization, post-processing, and enhanced functionality:

#### Ubuntu/Debian

```bash
sudo apt install -y gnuplot
sudo apt install -y imagemagick
sudo apt install -y ffmpeg
sudo apt install -y graphviz
sudo apt install -y valgrind
sudo apt install -y gifsicle
sudo apt install -y pstoedit
```

#### macOS

```bash
brew install gnuplot imagemagick ffmpeg graphviz
```

### Python Interface Dependencies

If you want to use Basilisk from Python:

```bash
sudo apt install -y swig
sudo apt install -y python3-dev
sudo apt install -y python3-numpy
sudo apt install -y python3-matplotlib
```

### MPI Support (for Parallel Computing)

```bash
sudo apt install -y libopenmpi-dev
sudo apt install -y openmpi-bin
```

### GPU Support (Optional)

For GPU-accelerated simulations:

```bash
# NVIDIA CUDA Toolkit
sudo apt install -y nvidia-cuda-toolkit
```

## Installation Steps

### Step 1: Clone or Download the Repository

If you haven't already, obtain the Basilisk source code:

```bash
git clone https://github.com/vinodthale/basilisk-local.git
cd basilisk-local
```

### Step 2: Navigate to Source Directory

```bash
cd src
```

### Step 3: Configure Build System

Select the appropriate configuration file for your system:

**For standard 64-bit Linux/macOS with GCC:**

```bash
ln -sf config.gcc config
```

**For 32-bit systems:**

```bash
ln -sf config.gcc.32bits config
```

**For OpenMP support:**

```bash
ln -sf config.gcc.openmp config
```

**For MPI support:**

```bash
ln -sf config.mpi config
```

### Step 4: Build Basilisk

```bash
make -j$(nproc)
```

The `-j$(nproc)` flag enables parallel compilation using all available CPU cores.

### Step 5: Set Environment Variables

Add Basilisk to your PATH and set the BASILISK environment variable:

**For Bash (most common):**

```bash
export BASILISK=$PWD
export PATH=$PATH:$BASILISK
echo "export BASILISK=$PWD" >> ~/.bashrc
echo 'export PATH=$PATH:$BASILISK' >> ~/.bashrc
source ~/.bashrc
```

**For Zsh:**

```bash
export BASILISK=$PWD
export PATH=$PATH:$BASILISK
echo "export BASILISK=$PWD" >> ~/.zshrc
echo 'export PATH=$PATH:$BASILISK' >> ~/.zshrc
source ~/.zshrc
```

### Step 6: Verify Installation

Check that the `qcc` compiler is available:

```bash
which qcc
qcc --version
```

You should see the path to qcc and version information.

## Optional Components

### Building Python Interface

After installing Python dependencies (see [Prerequisites](#python-interface-dependencies)):

```bash
cd src
make stream.py  # Build example Python module
```

To use Python with Basilisk, ensure `MDFLAGS` and `PYTHONINCLUDE` are set correctly in your `config` file.

### Building GL Utilities (for 3D visualization)

```bash
cd src/gl
make
```

### Building WebSocket Server (for interactive visualization)

```bash
cd src/wsServer
make
```

## Verification

### Test the Installation

Create a simple test file `test.c`:

```c
#include "grid/multigrid.h"
#include "navier-stokes/centered.h"

int main() {
  init_grid(128);
  run();
}
```

Compile and run:

```bash
qcc -o test test.c -lm
./test
```

If the compilation succeeds and the program runs without errors, your installation is working correctly.

### Run the Test Suite

To run the comprehensive test suite:

```bash
cd src
./runtest
```

**Note**: The full test suite can take several hours to complete.

## Troubleshooting

### Common Issues

#### Issue: `qcc: command not found`

**Solution**: Ensure you've added Basilisk to your PATH:

```bash
export BASILISK=/path/to/basilisk-local/src
export PATH=$PATH:$BASILISK
```

#### Issue: Compilation errors with missing headers

**Solution**: Ensure you have all prerequisites installed:

```bash
sudo apt install build-essential gawk
```

#### Issue: `make: *** No rule to make target 'all'`

**Solution**: Make sure you're in the `src` directory and have created the `config` symlink:

```bash
cd src
ln -sf config.gcc config
make
```

#### Issue: Python module import errors

**Solution**:
1. Verify SWIG and Python development headers are installed
2. Check that `MDFLAGS` in your config file includes Python paths
3. Ensure the `.so` file is in your Python path or current directory

#### Issue: MPI compilation fails

**Solution**:
1. Install OpenMPI development packages: `sudo apt install libopenmpi-dev`
2. Use the MPI configuration: `ln -sf config.mpi config`
3. Rebuild: `make clean && make`

### Getting Help

- **Documentation**: See `src/README` for detailed documentation
- **Tutorial**: http://basilisk.fr/Tutorial
- **Examples**: Browse `src/examples/` and `src/test/` directories
- **Issues**: Report bugs and issues on the project's issue tracker

## Next Steps

After successful installation:

1. Review the [Quick Start Guide](../README.md#quick-start)
2. Explore example simulations in `src/examples/`
3. Read the tutorial at http://basilisk.fr/Tutorial
4. Check out the technical guides in `docs/guides/`

---

**Last Updated**: November 2025
