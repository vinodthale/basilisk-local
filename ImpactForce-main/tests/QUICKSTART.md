# Quick Start Guide - Sharp VOF Test Suite

Get started with the validation test suite in 5 minutes!

## Prerequisites

1. **Basilisk installed** with `qcc` in PATH
2. **Python 3** with numpy, matplotlib (for analysis)

```bash
# Check Basilisk
which qcc

# Install Python packages (optional, for analysis)
pip3 install numpy matplotlib
```

## Quick Test Run

### 1. Navigate to tests directory

```bash
cd /path/to/basilisk-local/ImpactForce-main/tests
```

### 2. Run quick validation

```bash
#Run all tests in quick mode (~5-10 minutes)
./run_all_tests.sh --quick

# Or run specific test
cd validation
qcc -O2 test_mass_conservation.c -o test_mass_conservation -lm
./test_mass_conservation
```

### 3. Check results

```bash
# View results
ls -l results/

# Analyze
cd utilities
python3 analyze_results.py --all

# Plot (requires matplotlib)
python3 plot_validation.py --save
```

## Expected Output

### Successful Test Run

```
========================================
Sharp VOF Test Suite Runner
========================================
Mode: quick
Category: all
========================================

========================================
Category: validation
========================================

Running: test_mass_conservation (levels 9-10)
  ✓ PASSED

Running: test_contact_angle (levels 9-10)
  ✓ PASSED

========================================
Test Suite Summary
========================================
Total tests: 2
Passed: 2
Failed: 0
Duration: 120s
========================================

✓ ALL TESTS PASSED
========================================
```

### Results Files

```
results/
├── mass_conservation_L9-10.txt           # Volume data
├── mass_conservation_summary.json        # Test summary
├── contact_angle_90_deg.txt              # Angle measurements
├── contact_angle_90_summary.json         # Test summary
└── test_suite_summary.json               # Overall summary
```

## Understanding Test Results

### Mass Conservation (`mass_conservation_*.txt`)

```
# Time  Volume  Volume_Ratio  Abs_Error  Rel_Error
0.000   1.0000  1.000000     0.000e+00  0.000e+00
0.010   1.0000  1.000000     2.345e-12  2.345e-12
0.020   1.0000  0.999999     8.123e-12  8.123e-12
```

**Key columns**:
- `Volume_Ratio`: Should be ~1.000 (perfect conservation)
- `Rel_Error`: Should be < 1e-10 for Sharp VOF

**Pass criteria**: Max `Rel_Error` < 1e-10

### Contact Angle (`contact_angle_*.txt`)

```
# Time  Measured_Angle  Contact_Line_Pos
0.000   85.32          0.0000
0.050   88.45          0.1234
0.100   89.78          0.2456
```

**Key columns**:
- `Measured_Angle`: Should approach specified angle
- `Contact_Line_Pos`: Contact line position on substrate

**Pass criteria**: |Measured - Specified| < 5°

## Common Issues

### Issue 1: qcc not found

**Solution**:
```bash
cd ../../src
make
export PATH=$PATH:$(pwd)
```

### Issue 2: Compilation errors

**Solution**: Check Sharp VOF headers exist:
```bash
ls -la ../../2D-sharp-and-conservative-VOF-method-Basiliks-main/
```

### Issue 3: Test fails

**Check log**:
```bash
cat results/test_name_*.log
```

**Common causes**:
- Insufficient grid resolution → Increase `MAX_GRID_LEVEL`
- Unstable parameters → Reduce Reynolds/Weber numbers
- Tolerance too strict → Adjust in test file

## Test Modes

### Quick Mode (Recommended for first run)
```bash
./run_all_tests.sh --quick
```
- Grid levels: 9-10
- Duration: 5-15 minutes
- Use for: Testing setup, debugging

### Thorough Mode (Production validation)
```bash
./run_all_tests.sh --thorough
```
- Grid levels: 9-12
- Duration: 30-90 minutes
- Use for: Full validation, publications

### CI Mode (Automated testing)
```bash
./run_all_tests.sh --quick --ci
```
- Minimal output
- Exit codes: 0=pass, 1=fail, 2=critical fail

## Running Individual Tests

### Mass Conservation Test

```bash
cd validation
qcc -O2 test_mass_conservation.c -o test_mass_conservation -lm
./test_mass_conservation
```

**Parameters** (edit in `test_mass_conservation.c`):
- `MAX_TIME`: Simulation duration
- `SAVE_FILE_EVERY`: Output frequency
- Grid levels via command line: `./test_mass_conservation X11 N9`

### Contact Angle Test

```bash
cd validation
qcc -O2 test_contact_angle.c -o test_contact_angle -lm

# Test specific angle
./test_contact_angle A120  # Test 120° contact angle

# Default (90°)
./test_contact_angle
```

## Analysis Tools

### Python Analysis

```bash
cd utilities

# Load and analyze all results
python3 analyze_results.py --all

# Mass conservation only
python3 analyze_results.py --mass

# Contact angle only
python3 analyze_results.py --angle

# Generate text report
python3 analyze_results.py --report --output my_report.txt
```

### Python Plotting

```bash
cd utilities

# Plot and display
python3 plot_validation.py --all

# Plot and save to files
python3 plot_validation.py --all --save

# Specific plots
python3 plot_validation.py --mass --save
python3 plot_validation.py --angle --save
```

**Output files**:
- `mass_conservation_plot.png`
- `contact_angle_plot.png`
- `validation_summary.png`

## Customizing Tests

### Change Grid Levels

Edit test file or use command line:
```bash
# Command line
./test_mass_conservation X13 N10

# Or edit constants-sharp.h:
#define INITAL_GRID_LEVEL 10
#define MAX_GRID_LEVEL 13
```

### Change Physics Parameters

Edit `constants-sharp.h`:
```c
#define VELOCITY 5.00
#define DROP_DIAMETER 2.050e-03
#define REYNOLDS 1000.0
#define WEBER 100.0
```

### Change Contact Angle

For Sharp VOF tests:
```c
#define CONTACT_ANGLE_SUBSTRATE 120.0  // degrees
```

## Test Development

### Adding a New Test

1. Create `test_my_metric.c` in `validation/` or `benchmarks/`

2. Include Sharp VOF headers:
```c
#include "../../constants-sharp.h"
```

3. Implement test logic with these events:
```c
event init(i = 0) { /* Setup */ }
event measure(t += dt) { /* Measure metrics */ }
event end(t = MAX_TIME) { /* Report results */ }
```

4. Output results:
```c
FILE *fp = fopen("../results/my_metric.txt", "w");
fprintf(fp, "# My Metric Results\n");
fprintf(fp, "%.6f  %.6e\n", time, metric);
```

5. Add to `run_all_tests.sh`

6. Document in `README.md`

## Performance Tips

### Faster Tests

```bash
# Reduce grid resolution
# Edit test file:
#define MAX_GRID_LEVEL 11  # Instead of 12

# Shorter duration
#define MAX_TIME 1.0  # Instead of 2.0

# Less frequent output
#define SAVE_FILE_EVERY 0.05  # Instead of 0.01
```

### Parallel Tests

```bash
# Run tests in parallel (GNU parallel)
parallel -j 4 ::: ./test_*

# Or MPI version of ImpactForce
# (Test suite itself is serial)
```

## Interpreting Results

### Test Status

- **✅ PASS**: All criteria met
- **❌ FAIL**: One or more criteria failed
- **⚠️ WARNING**: Borderline results

### Summary JSON

```json
{
  "test_name": "mass_conservation",
  "status": "PASS",
  "metrics": {
    "max_volume_error": 1.23e-11,
    "mean_relative_error": 4.56e-12
  },
  "parameters": {
    "Re": 1000,
    "We": 100,
    "levels": "9-12"
  }
}
```

## Next Steps

1. **Run quick test** to verify setup ✅
2. **Check results** in `results/` directory
3. **Run thorough test** for full validation
4. **Analyze and plot** using Python scripts
5. **Read full documentation** in `README.md`
6. **Customize tests** for your parameters
7. **Add new tests** as needed

## Getting Help

### Check Logs
```bash
# View test output
cat results/test_*.log

# Check compilation
cat results/test_*_compile.log
```

### Debug Test
```bash
# Run test directly with verbose output
cd validation
./test_mass_conservation 2>&1 | tee debug.log
```

### Common Solutions

| Problem | Solution |
|---------|----------|
| qcc not found | Build Basilisk, add to PATH |
| Headers not found | Check Sharp VOF directory location |
| Test crashes | Reduce grid levels, check parameters |
| Mass not conserved | Increase resolution, check TOLERANCE |
| Angle wrong | Verify CONTACT_ANGLE_SUBSTRATE setting |
| Slow tests | Use --quick mode, reduce MAX_GRID_LEVEL |

## Summary Commands

```bash
# Quick test run
./run_all_tests.sh --quick

# Full validation
./run_all_tests.sh --thorough

# Analyze results
cd utilities && python3 analyze_results.py --all

# Plot results
python3 plot_validation.py --all --save

# Generate report
python3 analyze_results.py --report

# Clean results
rm -rf results/*.txt results/*.json results/*.log
```

---

**Ready?** Run `./run_all_tests.sh --quick` to get started! 🚀
