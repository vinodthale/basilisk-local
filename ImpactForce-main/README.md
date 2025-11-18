# Impact Force Simulations

Code to simulate the impact of an air-in-liquid compound drop using the Basilisk flow solver (DNS).

## Project Structure

```
ImpactForce-main/
├── README.md                 # This file
├── src/                      # Source code
│   ├── Bdropimpact.c        # Drop impact simulation
│   ├── Bdropimpact-sharp.c  # Sharp VOF method variant
│   ├── Jetandpressure.c     # Jet and pressure analysis
│   ├── constants.h          # Constants and parameters
│   ├── constants-sharp.h    # Constants for sharp VOF
│   └── scripts/             # Build and run scripts
│       ├── ClusterMPI.sh
│       ├── RJetandpressure.sh
│       ├── compile-sharp-mpi.sh
│       └── compile-sharp.sh
├── tests/                   # Test suite
│   ├── validation/          # Basic validation tests
│   ├── drop_vaporization_nondim/  # Drop vaporization tests
│   ├── paper_validation_2402.10185/  # Paper validation suite
│   └── utilities/           # Test analysis utilities
└── docs/                    # Documentation
    ├── 2402.10185v1.pdf
    ├── INTEGRATION-GUIDE.md
    ├── README-SHARP-VOF.md
    └── TEST-SUITE-SUMMARY.md
```

## Installation

The code relies on the Basilisk flow solver to simulate the impact forces of air-in-liquid compound drops onto a substrate.

More information about the Basilisk solver: http://basilisk.dalembert.upmc.fr/

## Running the Code

### Basic Simulation

```bash
cd src/scripts
./ClusterMPI.sh
```

### Compilation

For standard VOF method:
```bash
cd src/scripts
./compile-sharp.sh
```

For MPI parallel execution:
```bash
cd src/scripts
./compile-sharp-mpi.sh
```

## Running Tests

See [tests/README.md](tests/README.md) for detailed testing instructions.

Quick start:
```bash
cd tests
./run_all_tests.sh
```

## Documentation

Additional documentation is available in the `docs/` directory:

- **INTEGRATION-GUIDE.md**: Integration and setup guide
- **README-SHARP-VOF.md**: Sharp VOF method documentation
- **TEST-SUITE-SUMMARY.md**: Comprehensive test suite documentation
- **2402.10185v1.pdf**: Reference paper 
