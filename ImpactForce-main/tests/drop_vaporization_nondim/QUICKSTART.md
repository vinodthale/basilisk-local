# Quick Start Guide: Nondimensional Drop Vaporization Test

## 1. Prerequisites

### Install Basilisk

```bash
# Clone Basilisk
git clone https://github.com/basilisk-fr/basilisk.git
cd basilisk/src
make

# Set environment variables (add to ~/.bashrc)
export BASILISK=$HOME/basilisk/src
export PATH=$PATH:$BASILISK
```

### Install Python dependencies

```bash
pip3 install numpy matplotlib
```

## 2. Verify Installation

```bash
# Check Basilisk compiler
which qcc
qcc --version

# Check Python
python3 -c "import numpy, matplotlib; print('OK')"
```

## 3. Run Your First Test

### Single case (Re = 100)

```bash
cd ImpactForce-main/tests/drop_vaporization_nondim/utilities
./run_single_case.sh 100
```

**Expected output**:
```
========================================
Nondimensional Drop Vaporization Test
========================================

Parameters:
  Re = 100.00
  We = 1.50 (fixed)
  Lmax = 12

Output directory:
  ../results/Re_100.00_We_1.50

========================================
Compiling...
========================================
...
```

**Runtime**: ~10-30 minutes (depending on your system)

## 4. Check Results

```bash
# View volume history
cat ../results/Re_100.00_We_1.50/liquid_volume_history.txt

# Plot results
python3 analyze_results.py --plot-volume --save ../results/Re_100.00_We_1.50
```

## 5. Quick Parameter Sweep (5 cases)

```bash
./run_parameter_sweep.sh --quick
```

This runs 5 cases: Re = 22, 66.5, 111, 155.5, 200

**Runtime**: ~1-2 hours

## 6. Analyze and Compare

```bash
# Compare all cases
python3 analyze_results.py --compare --save ../results
```

This generates:
- `volume_comparison.png`: Volume evolution for all Re
- `re_scaling.png`: Vaporization rate vs Re

## 7. Full Parameter Sweep (25 cases)

```bash
# Sequential execution
./run_parameter_sweep.sh

# Parallel execution (4 jobs)
./run_parameter_sweep.sh --parallel 4
```

**Runtime**: ~5-10 hours (sequential), ~2-3 hours (parallel with 4 cores)

## Common Issues

### "qcc: command not found"

**Solution**: Add Basilisk to your PATH:
```bash
export PATH=$PATH:$BASILISK
```

### Compilation error: "axi.h not found"

**Solution**: Set BASILISK environment variable:
```bash
export BASILISK=/path/to/basilisk/src
```

### Simulation crashes with "Out of memory"

**Solution**: Reduce refinement level:
```bash
./run_parameter_sweep.sh --lmax 10
```

### Slow execution

**Solution**: Use quick mode or lower refinement:
```bash
./run_parameter_sweep.sh --quick --lmax 10
```

## Next Steps

1. **Visualize with ParaView**: Open VTK files in `results/Re_XXX_We_XXX/`
2. **Customize parameters**: Edit `test_drop_vap_nondim.c`
3. **Run convergence study**: Test with Lmax = 10, 11, 12
4. **Compare with reference data**: Check Boyd & Ling 2023, Figure 10

## Example Workflow

```bash
# 1. Quick test with low refinement
cd utilities
./run_single_case.sh 100
# Edit test_drop_vap_nondim.c: set Lmax = 10
./run_single_case.sh 100

# 2. Run quick sweep
./run_parameter_sweep.sh --quick --lmax 10

# 3. Analyze
python3 analyze_results.py --compare --save ../results

# 4. If results look good, run full sweep
./run_parameter_sweep.sh --parallel 4
```

## Expected Results

| Re | V/V₀ at t*=0.16 | Vaporization Rate |
|----|-----------------|-------------------|
| 22 | ~0.95 | Low |
| 100 | ~0.85 | Moderate |
| 200 | ~0.75 | High |

*Higher Re → faster vaporization due to enhanced heat transfer*

## Support

- **README**: See `README.md` for full documentation
- **Logs**: Check `results/Re_XXX_We_XXX/log.txt` for errors
- **Basilisk docs**: http://basilisk.fr/Tutorial
