# ImpactForce Sharp VOF Test Suite

Comprehensive verification and validation test suite for the Sharp VOF integration into ImpactForce, based on standard drop impact benchmarks and comparison with reference paper (arXiv:2402.10185).

## Overview

This test suite validates:
- **Mass/Volume Conservation**: VOF field conservation
- **Contact Angle Accuracy**: Dynamic contact line behavior
- **Interface Sharpness**: Numerical diffusion metrics
- **Grid Convergence**: Spatial resolution independence
- **Energy Conservation**: Kinetic + potential energy tracking
- **Impact Force Validation**: Pressure-based force calculations
- **Spreading Dynamics**: Contact line position and velocity
- **Comparison Tests**: Sharp VOF vs Standard VOF

## Directory Structure

```
tests/
├── README.md                    # This file
├── run_all_tests.sh             # Master test runner
├── validation/                  # Validation test cases
│   ├── test_mass_conservation.c
│   ├── test_contact_angle.c
│   ├── test_interface_sharpness.c
│   ├── test_grid_convergence.c
│   ├── test_energy_conservation.c
│   ├── test_impact_force.c
│   └── test_spreading_dynamics.c
├── benchmarks/                  # Standard benchmark problems
│   ├── static_droplet.c         # Equilibrium contact angle
│   ├── capillary_wave.c         # Surface tension validation
│   ├── poiseuille_flow.c        # Viscosity validation
│   └── zalesak_disk.c           # VOF advection test
├── utilities/                   # Analysis tools
│   ├── analyze_results.py       # Python analysis script
│   ├── plot_validation.py       # Plotting utilities
│   ├── compare_versions.py      # Compare Sharp vs Standard
│   └── extract_metrics.sh       # Extract metrics from logs
├── results/                     # Test results (gitignored)
│   └── .gitkeep
└── reference_data/              # Reference solutions
    ├── mass_conservation_ref.txt
    ├── contact_angle_ref.txt
    └── spreading_ref.txt
```

## Quick Start

### Run All Tests

```bash
cd tests
./run_all_tests.sh
```

### Run Individual Test

```bash
# Compile and run specific test
cd validation
qcc -O2 test_mass_conservation.c -o test_mass_conservation -lm
./test_mass_conservation

# Analyze results
cd ../utilities
python3 analyze_results.py ../results/mass_conservation_*.txt
```

### Run Comparison Test

```bash
# Compare Sharp VOF vs Standard VOF
./run_comparison_test.sh R1000 W100
```

## Test Cases

### 1. Mass Conservation Test (`test_mass_conservation.c`)

**Purpose**: Verify VOF field is strictly conserved

**Metrics**:
- Volume ratio over time: V(t)/V(0)
- Absolute error: |V(t) - V(0)|
- Relative error: |V(t) - V(0)|/V(0)

**Pass Criteria**:
- ✅ |ΔV/V₀| < 1e-10 for Sharp VOF
- ✅ |ΔV/V₀| < 1e-8 for Standard VOF

**Parameters**:
- Re = 1000, We = 100
- Grid: 9-12 levels
- Time: 0 to 2.0

### 2. Contact Angle Test (`test_contact_angle.c`)

**Purpose**: Validate contact angle implementation

**Metrics**:
- Measured contact angle at substrate
- Contact angle evolution
- Hysteresis behavior

**Pass Criteria**:
- ✅ |θ_measured - θ_specified| < 5°
- ✅ Stable equilibrium angle within 10% of specified
- ✅ Contact line does not penetrate solid

**Parameters**:
- Contact angles: 30°, 60°, 90°, 120°, 150°
- Static droplet on substrate
- Time to equilibrium

### 3. Interface Sharpness Test (`test_interface_sharpness.c`)

**Purpose**: Measure numerical diffusion of interface

**Metrics**:
- Interface thickness: δ = ∫|∇f| dA / L
- Sharpness parameter: S = max(|∇f|)
- Comparison with initial profile

**Pass Criteria**:
- ✅ δ/Δx < 3 (interface < 3 cells thick)
- ✅ Sharp VOF sharper than Standard VOF
- ✅ No excessive smoothing over time

**Parameters**:
- Advection of spherical droplet
- Grid: 9-12 levels
- Time: 1-2 characteristic times

### 4. Grid Convergence Test (`test_grid_convergence.c`)

**Purpose**: Verify solution converges with grid refinement

**Metrics**:
- Max velocity error: ||u_h - u_2h||
- Spreading radius at fixed time
- Convergence order: p = log(E_h/E_2h)/log(2)

**Pass Criteria**:
- ✅ Convergence order p > 1.5
- ✅ Solution stable at finest grid
- ✅ No grid-dependent artifacts

**Parameters**:
- Grid levels: 8-9, 9-10, 10-11, 11-12
- Re = 1000, We = 100
- Compare at t = 0.5, 1.0, 1.5

### 5. Energy Conservation Test (`test_energy_conservation.c`)

**Purpose**: Track total energy evolution

**Metrics**:
- Kinetic energy: KE = ∫½ρu² dV
- Potential energy: PE = ∫ρgz dV
- Surface energy: SE = ∫σ dA
- Total energy: E_total = KE + PE + SE
- Dissipation: D = ∫μ(∇u)² dV

**Pass Criteria**:
- ✅ E_total + ∫D dt ≈ E₀ (energy balance)
- ✅ Monotonic decrease of total energy
- ✅ Smooth energy evolution (no jumps)

**Parameters**:
- Re = 100, 1000 (viscous, inertial)
- We = 50, 100, 200
- Track from impact to spreading

### 6. Impact Force Test (`test_impact_force.c`)

**Purpose**: Validate pressure-based force calculation

**Metrics**:
- Peak impact force: F_max
- Time to peak force: t_peak
- Force impulse: I = ∫F dt
- Comparison with momentum change

**Pass Criteria**:
- ✅ F_max agrees with literature (±20%)
- ✅ Force impulse matches momentum change (±10%)
- ✅ No spurious oscillations in force

**Parameters**:
- Re = 500, 1000, 2000
- We = 50, 100, 200
- Pool depths: 0, 0.5D, 1.0D

### 7. Spreading Dynamics Test (`test_spreading_dynamics.c`)

**Purpose**: Measure contact line motion

**Metrics**:
- Contact line position: r_cl(t)
- Spreading factor: β(t) = r_cl(t)/R₀
- Maximum spreading: β_max
- Spreading velocity: dr_cl/dt

**Pass Criteria**:
- ✅ β_max matches correlations (±15%)
- ✅ β ~ t^α with correct exponent
- ✅ Smooth contact line motion

**Parameters**:
- We = 10, 50, 100, 200
- Re = 100, 1000
- θ = 30°, 90°, 150°

### 8. Benchmark: Static Droplet (`static_droplet.c`)

**Purpose**: Equilibrium contact angle on substrate

**Metrics**:
- Contact angle at equilibrium
- Droplet shape (spherical cap)
- Pressure jump across interface

**Pass Criteria**:
- ✅ Equilibrium angle = specified (±2°)
- ✅ Laplace pressure: Δp = σ/R (±5%)
- ✅ No spurious currents (|u| < 1e-6)

### 9. Benchmark: Capillary Wave (`capillary_wave.c`)

**Purpose**: Surface tension validation

**Metrics**:
- Wave frequency: ω = √(σk³/ρ)
- Damping rate: γ = 2μk²/ρ
- Amplitude decay

**Pass Criteria**:
- ✅ Frequency error < 5%
- ✅ Damping matches theory (±10%)
- ✅ No numerical instabilities

### 10. Benchmark: Zalesak's Disk (`zalesak_disk.c`)

**Purpose**: Pure advection without diffusion

**Metrics**:
- Shape preservation after rotation
- Volume conservation
- Interface sharpness

**Pass Criteria**:
- ✅ Volume error < 0.1%
- ✅ Shape well preserved
- ✅ Sharp VOF better than Standard

## Running Tests

### Prerequisites

```bash
# Ensure Basilisk is built
cd ../../src
make
export PATH=$PATH:$(pwd)

# Install Python dependencies (for analysis)
pip3 install numpy matplotlib scipy pandas
```

### Full Test Suite

```bash
cd tests
./run_all_tests.sh

# Options:
./run_all_tests.sh --quick       # Fast tests only (levels 9-10)
./run_all_tests.sh --thorough    # Full tests (levels 9-12)
./run_all_tests.sh --validation  # Validation tests only
./run_all_tests.sh --benchmarks  # Benchmark tests only
```

### Individual Test Categories

```bash
# Validation tests
cd validation
make all
make run

# Benchmarks
cd benchmarks
make all
make run

# Analysis
cd utilities
python3 analyze_results.py --all
python3 plot_validation.py --save
```

### Comparison Tests

```bash
# Compare Sharp VOF vs Standard VOF
./run_comparison_test.sh

# Specific parameters
./run_comparison_test.sh R1000 W100 X12 N9

# Generate comparison report
cd utilities
python3 compare_versions.py --generate-report
```

## Output and Results

### Test Output Format

Each test generates:

1. **Log file**: `test_name_YYYYMMDD_HHMMSS.log`
2. **Data file**: `test_name_results.txt`
3. **Snapshot files**: `test_name_snapshot_*.dat`
4. **Summary**: `test_name_summary.json`

### Example Data File

```
# Mass Conservation Test Results
# Time  Volume  Volume_Ratio  Error
0.000   1.0000  1.000000     0.000e+00
0.010   1.0000  1.000000     2.345e-12
0.020   1.0000  0.999999     8.123e-12
...
```

### Summary JSON

```json
{
  "test_name": "mass_conservation",
  "timestamp": "2025-01-17T10:30:00",
  "status": "PASS",
  "metrics": {
    "max_volume_error": 1.23e-10,
    "mean_volume_error": 4.56e-11,
    "final_volume_ratio": 0.999999999
  },
  "parameters": {
    "Re": 1000,
    "We": 100,
    "levels": "9-12"
  }
}
```

## Analysis Tools

### Python Scripts

```bash
# Analyze all results
python3 utilities/analyze_results.py results/*.txt

# Plot validation metrics
python3 utilities/plot_validation.py --metric volume --time

# Compare versions
python3 utilities/compare_versions.py \
  results/sharp_vof_*.txt \
  results/standard_vof_*.txt

# Generate report
python3 utilities/analyze_results.py --generate-report --pdf
```

### Shell Scripts

```bash
# Extract metrics from logs
./utilities/extract_metrics.sh results/test_*.log

# Quick summary
./utilities/test_summary.sh
```

## Validation Criteria

### Overall Test Suite Pass

✅ **PASS** if:
- All individual tests pass
- No critical failures
- Performance within expected range

⚠️ **WARNING** if:
- Some tests borderline
- Performance degraded
- Minor accuracy issues

❌ **FAIL** if:
- Any critical test fails
- Mass conservation violated
- Numerical instabilities

### Test Classification

- **Critical**: Mass conservation, stability
- **Important**: Contact angle, grid convergence
- **Performance**: Runtime, memory usage
- **Optional**: Advanced metrics, comparisons

## Continuous Integration

Tests can be run automatically:

```bash
# Add to CI pipeline
cd tests
./run_all_tests.sh --quick --ci

# Exit codes:
# 0 = All tests passed
# 1 = Some tests failed
# 2 = Critical test failed
```

## Troubleshooting

### Test Fails to Compile

```bash
# Check Basilisk path
which qcc

# Check includes
ls -la ../2D-sharp-and-conservative-VOF-method-Basiliks-main/
```

### Test Runs but Fails

```bash
# Check log file
cat results/test_name_*.log

# Visualize results
python3 utilities/plot_validation.py results/test_name_results.txt
```

### Performance Issues

```bash
# Run quick test first
./run_all_tests.sh --quick

# Reduce grid levels in test file
# Edit: #define MAX_GRID_LEVEL 11  # Instead of 12
```

## Reference Data

Reference solutions in `reference_data/` from:
- Literature values (papers)
- Analytical solutions (where available)
- Benchmark databases

## Contributing

To add a new test:

1. Create test file in `validation/` or `benchmarks/`
2. Follow naming convention: `test_*.c`
3. Output results in standard format
4. Add to `run_all_tests.sh`
5. Document in this README

## Citation

If using this test suite, please cite:
- ImpactForce module (Thale et al.)
- Sharp VOF method (Huang et al., 2025)
- Reference paper: arXiv:2402.10185

## Contact

For issues or questions about the test suite:
- Check logs in `results/`
- Review documentation
- Check GitHub issues

---

**Last Updated**: 2025-01-17
**Version**: 1.0
**Status**: Active Development
