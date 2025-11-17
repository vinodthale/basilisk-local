# Validation Suite for arXiv:2402.10185

## Paper Reference

**Title**: "A coupled VOF/embedded boundary method to model two-phase flows on arbitrary solid surfaces"

**Authors**: Mathilde Tavares, Christophe Josserand, Alexandre Limare, José María Lopez-Herrera, Stéphane Popinet

**Institution**: LadHyX École Polytechnique, Institut Jean Le Rond d'Alembert (CNRS & Sorbonne University), Universidad de Sevilla, ArianeGroup

**Date**: February 16, 2024

**arXiv**: [2402.10185v1](https://arxiv.org/abs/2402.10185)

**DOI**: [10.1016/j.jcp.2025.113975](https://doi.org/10.1016/j.jcp.2025.113975)

---

## Overview

This test suite implements comprehensive validation cases from the Tavares et al. (2024) paper to verify the implementation of:
- **VOF/Embedded Boundary Coupling**: Hybrid method for two-phase flows on complex geometries
- **Contact Angle Boundary Conditions**: Geometric approach for wetting dynamics
- **Moving Contact Lines**: Navier slip and no-slip boundary conditions
- **Mass Conservation**: Validation of volume conservation in three-phase systems

### Implementation Details

**Contact Angle Method**: These tests use the **Sharp VOF** implementation for contact angles on embedded boundaries,
based on Huang et al. (2025) "A 2D sharp and conservative VOF method for modeling the contact line dynamics
with hysteresis on complex boundary" (J. Comput. Phys.).

This implementation differs from standard Basilisk `contact.h` which only supports contact angles on flat
grid-aligned boundaries. The Sharp VOF method uses a ghost cell approach to impose contact angles on arbitrary
embedded boundaries, as described in the Tavares et al. paper.

**2D Simplifications**: All tests use 2D Cartesian grids (`quadtree`). Some physical configurations that are
inherently 3D or axisymmetric (e.g., capillary tubes, cylindrical films) are approximated as 2D cross-sections
or equivalent 2D geometries for computational efficiency.

---

## Quick Start

### Run All Implemented Tests

```bash
cd /home/user/basilisk-local/ImpactForce-main/tests/paper_validation_2402.10185

# Quick validation (~15 minutes)
./run_paper_tests.sh --quick

# Paper-quality resolution (~2 hours)
./run_paper_tests.sh --paper-quality
```

### Run Specific Test

```bash
# Test 1: Static contact angle
./run_paper_tests.sh --test 1

# Test 3: Droplet impact
./run_paper_tests.sh --test 3
```

### Analyze Results

```bash
cd analysis

# Compare with paper data
python3 compare_with_paper.py --report

# Generate plots
python3 plot_comparisons.py --all --save
```

---

## Implementation Status

| Test | Status | Features |
|------|--------|----------|
| **Test 1: Static Contact Angle** | ✅ **IMPLEMENTED** | Equilibrium angle, spherical cap geometry, Laplace pressure, spurious currents |
| **Test 2: Sliding Droplet** | ✅ **IMPLEMENTED** | Inclined plane, hysteresis, sliding velocity |
| **Test 3: Droplet Impact** | ✅ **IMPLEMENTED** | Maximum spreading, correlations, mass conservation, dynamic angle |
| **Test 4: Capillary Rise** | ✅ **IMPLEMENTED** | Meniscus shape, rise height, Lucas-Washburn |
| **Test 5: Film Flow** | ✅ **IMPLEMENTED** | Film thickness, curved surface, gravity drainage |
| **Test 6: Cylinder Wetting** | ✅ **IMPLEMENTED** | 3D geometry, wetting transition, surface curvature |

---

## Directory Structure

```
paper_validation_2402.10185/
├── README.md                           # This file
├── PAPER-VALIDATION-SUMMARY.md         # Detailed validation summary
├── run_paper_tests.sh                  # Master test runner
├── test_cases/                         # Test implementations
│   ├── test_static_contact_angle.c     # Case 1: Equilibrium angle ✅ IMPLEMENTED
│   ├── test_sliding_droplet.c          # Case 2: Sliding on incline ✅ IMPLEMENTED
│   ├── test_droplet_impact.c           # Case 3: Impact dynamics ✅ IMPLEMENTED
│   ├── test_capillary_rise.c           # Case 4: Rise in tube ✅ IMPLEMENTED
│   ├── test_film_flow.c                # Case 5: Film on curved surface ✅ IMPLEMENTED
│   └── test_cylinder_wetting.c         # Case 6: Droplet on cylinder ✅ IMPLEMENTED
├── reference_data/                     # Data from paper
│   ├── paper_fig_*.txt                 # Digitized figures
│   ├── experimental_data.txt           # Experimental comparisons
│   └── theoretical_solutions.txt       # Analytical solutions
├── results/                            # Test outputs (gitignored)
│   ├── sliding_droplet/
│   ├── capillary_rise/
│   ├── film_flow/
│   └── cylinder_wetting/
├── analysis/                           # Comparison scripts
│   ├── compare_with_paper.py           # Compare against paper data
│   ├── plot_comparisons.py             # Generate comparison plots
│   └── generate_paper_report.py        # Create validation report
└── docs/                               # Documentation
    ├── test_protocols.md               # Test procedures
    └── validation_criteria.md          # Pass/fail criteria
```

---

## Test Cases

### Case 1: Static Contact Angle

**File**: `test_static_contact_angle.c`
**Paper Reference**: Section 4.2, Figure 3
**Description**: Droplet relaxes to equilibrium contact angle on flat embedded plane

**Test Matrix**:
- Contact angles: 30°, 60°, 90°, 120°, 150°
- Grid levels: 7-10
- Plane orientations: 0° (horizontal), 45° (inclined)
- Bo (Bond number): 0.1 (nearly spherical)

**Pass Criteria**:
- |θ_measured - θ_specified| < 5°
- Mass absorption < 5%

**Validation Metrics**:
- Measured contact angle vs specified: |θ - θ₀| < 2°
- Droplet height/radius ratio matches spherical cap theory
- Laplace pressure: Δp = σ/R (within 5%)
- No spurious currents: |u_max| < 1e-6

---

### Case 2: Sliding Droplet on Inclined Plane

**File**: `test_sliding_droplet.c`
**Paper Reference**: Section 4.2
**Description**: 2D droplet on embedded plane with various inclinations

**Physical Parameters**:
- Initial radius: R₀ = 0.5
- Initial contact angle: θᵢ = 90° (half-disk)
- Static contact angle: θₛ (variable)
- Eötvös number: Eo = ρgR₀²/σ (0 or >0)

**Analytical Solution** (Eo=0):
```
Rf = R0 * sqrt(π / (2(θs - sin(θs)cos(θs))))
rf = Rf * sin(θs)  (spreading radius)
hf = Rf * (1 - cos(θs))  (droplet height)
```

**Test Matrix**:
```bash
# Run with different parameters
qcc -O2 test_sliding_droplet.c -o test_sliding_droplet -lm

# Horizontal plane, 60° contact angle
./test_sliding_droplet A60 P0 L8

# Inclined 45° plane, 90° contact angle
./test_sliding_droplet A90 P45 L8

# With gravity (Eo=10)
./test_sliding_droplet A60 P0 E10 L8
```

**Arguments**:
- `A###`: Contact angle (degrees)
- `P###`: Plane inclination angle (degrees)
- `E###`: Eötvös number
- `L##`: Maximum grid level

**Pass Criteria**:
- Radius error < 5%
- Height error < 5%
- Contact angle error < 5°
- Mass absorption < 5%

**Expected Issues** (from paper):
- Contact line pinning on 45° inclined plane for extreme angles (15°, 165°)
- Mesh-dependent numerical slip for no-slip BC

---

### Case 3: Droplet Impact

**File**: `test_droplet_impact.c`
**Paper Reference**: Section 3.3, Figure 7
**Description**: Normal impact on horizontal substrate with maximum spreading validation

**Physical Parameters**:
- We (Weber number): 10, 50, 100, 200
- Re (Reynolds number): 100, 500, 1000, 2000
- Contact angles: 90°, 120°, 150°
- Impact velocity: 0.5 - 5.0 m/s

**Validation Metrics**:
- Maximum spreading factor: β_max = D_max/D₀
- Time to maximum spreading: t_max
- Comparison with correlations:
  - β_max ~ We^(1/4) (inviscid)
  - β_max ~ Re^(1/5) (viscous)

**Correlations**:
- Pasandideh-Fard et al. (1996): β_max = √(We + 12)/(3(1-cos θ) + 4We/√Re)
- Clanet et al. (2004): β_max = 0.87 Re^(1/5) - 0.40 Re^(2/5) We^(-1)

**Pass Criteria**:
- Maximum spreading error < 5%
- Mass conservation < 1%
- Contact line dynamics match expected behavior

---

### Case 4: Capillary Rise in Tube

**File**: `test_capillary_rise.c`
**Paper Reference**: Section 3.4, Figure 9
**Description**: Liquid rises in vertical circular tube due to wetting

**Physical Parameters**:
- Tube radius: R_tube = 0.2 m (or 0.5-2.0 mm for other tests)
- Contact angle: θ
- Surface tension: σ = 1.0 N/m
- Liquid density: ρ_L = 1000 kg/m³
- Gravity: g = 9.81 m/s²

**Analytical Solution** (Jurin's Law):
```
h = (2σ cos(θ)) / (ρ g R_tube)
```

**Test Matrix**:
```bash
qcc -O2 test_capillary_rise.c -o test_capillary_rise -lm

# Different contact angles
./test_capillary_rise A30 L9  # Hydrophilic
./test_capillary_rise A60 L9
./test_capillary_rise A75 L9  # Close to neutral
```

**Arguments**:
- `A##`: Contact angle (degrees)
- `L##`: Maximum grid level

**Pass Criteria**:
- Height error < 10% (relaxed due to dynamics)
- Contact angle error < 5°
- Mass conservation < 1%

**Validation Metrics**:
- Equilibrium height vs. Jurin's Law
- Meniscus shape (spherical cap)
- Oscillation damping rate
- Lucas-Washburn dynamics: h ~ √t

---

### Case 5: Film Flow on Curved Surface

**File**: `test_film_flow.c`
**Paper Reference**: Section 3.5, Figure 11
**Description**: Gravity-driven thin film on curved embedded boundary

**Physical Parameters**:
- Substrate: Horizontal cylinder (R=1.0) or sphere
- Film thickness: h₀ = 0.1 (or 0.5 mm)
- Contact angle: θ
- Bond number: Bo = ρgh²/σ
- Capillary number: Ca = μU/σ
- Oh (Ohnesorge number): 0.01 - 0.1

**Test Matrix**:
```bash
qcc -O2 test_film_flow.c -o test_film_flow -lm

# Cylinder geometry, 60° contact angle
./test_film_flow A60 G0 L8

# Sphere geometry, 90° contact angle
./test_film_flow A90 G1 L8
```

**Arguments**:
- `A##`: Contact angle (degrees)
- `G#`: Geometry type (0=cylinder, 1=sphere)
- `L##`: Maximum grid level

**Pass Criteria**:
- Contact angle maintained within 10°
- Mass conservation < 3%
- No numerical instabilities

**Validation Metrics**:
- Film thickness profile
- Contact line velocity
- Drainage rate
- Rayleigh-Plateau instability wavelength

---

### Case 6: Droplet on Cylinder

**File**: `test_cylinder_wetting.c`
**Paper Reference**: Section 4.1, Figures 9-13
**Description**: 2D droplet spreading on horizontal embedded cylinder

**Physical Parameters**:
- Initial droplet radius: R₀ = 0.5
- Cylinder radius: Rc = 0.5 (or 1 mm)
- Initial contact angle: θᵢ = 90°
- Static contact angle: θₛ (variable)
- No gravity (Eo = 0)
- Bo = 0.1

**Analytical Solution**:
Volume conservation determines equilibrium:
```
S0 = πR0² - Rc²*arccos(...) - R0²*arccos(...) + sqrt(...)
```
Solved numerically from Eqs. 53-55 in paper.

**Test Matrix**:
```bash
qcc -O2 test_cylinder_wetting.c -o test_cylinder_wetting -lm

# Various contact angles with different grid levels
./test_cylinder_wetting A30 L6   # N=64
./test_cylinder_wetting A60 L7   # N=128
./test_cylinder_wetting A90 L8   # N=256
./test_cylinder_wetting A120 L7
./test_cylinder_wetting A150 L7
```

**Arguments**:
- `A###`: Contact angle (degrees)
- `L##`: Maximum grid level

**Pass Criteria**:
- Contact angle error < 10° (relaxed)
- Mass absorption < 5%
- Shape qualitatively matches analytical

**Expected Results** (from paper, Fig. 9-12):
- Good agreement for moderate angles (60°-120°)
- Some deviation for extreme angles (30°, 150°)
- First-order convergence with grid refinement
- Mass absorption converges at ~1-5% depending on angle

---

## Running Tests

### Compilation Requirements

**IMPORTANT**: These tests require the Sharp VOF implementation for embedded contact angles.
The standard Basilisk `contact.h` does NOT support embedded boundaries.

The tests are configured to use the Sharp VOF headers from:
`../../../2D-sharp-and-conservative-VOF-method-Basiliks-main/`

### Individual Test
```bash
cd test_cases

# Compile specific test (Sharp VOF headers included automatically)
qcc -O2 test_sliding_droplet.c -o test_sliding_droplet -lm

# Run with parameters
./test_sliding_droplet A60 P0 L8

# Check results
cat ../results/sliding_droplet/angle60_plane0_eo0.00.txt
```

**Note**: If compilation fails with "file not found" errors, verify that the Sharp VOF
implementation is available at the expected path relative to the test directory.

### All Tests
```bash
# Run complete test suite
./run_paper_tests.sh

# Quick tests only (low resolution)
./run_paper_tests.sh --quick

# Full validation (high resolution, paper resolution)
./run_paper_tests.sh --paper-quality
```

---

## Output Format

Each test generates:

1. **Console output**: Real-time progress and final results
2. **Data file**: Time series of validation metrics
3. **Status**: PASS/FAIL based on criteria

### Example Output
```
========================================
Sliding Droplet Test (Paper Section 4.2)
========================================
Plane angle: 0.0°
Contact angle: 60.0°
Eötvös number: 0.00
Max level: 8
----------------------------------------
Analytical predictions:
  Rf = 0.6204
  rf = 0.5372
  hf = 0.3102
========================================

t=  0.000: r=0.5000 h=0.5000 θ=90.0° Δm=0.00%
t=  0.050: r=0.5234 h=0.4123 θ=72.3° Δm=0.12%
...
t=  5.000: r=0.5368 h=0.3115 θ=60.8° Δm=2.34%

========================================
FINAL RESULTS
========================================
Radius:
  Measured:   0.5368
  Analytical: 0.5372
  Error:      0.07%
Height:
  Measured:   0.3115
  Analytical: 0.3102
  Error:      0.42%
Contact angle:
  Measured:   60.8°
  Specified:  60.0°
  Error:      0.8°
========================================

TEST STATUS: ✅ PASS
========================================
```

---

## Validation Against Paper

### Figure 9 (Section 4.1): Droplet on Cylinder
**Test**: `test_cylinder_wetting.c`
**Reproduce**:
```bash
for angle in 30 60 90 120; do
    ./test_cylinder_wetting A${angle} L7
done
```
**Expected**: Droplet shapes match analytical solution; better agreement for 60°-120°

### Figure 10 (Section 4.1): Grid Convergence
**Test**: `test_cylinder_wetting.c`
**Reproduce**:
```bash
for level in 6 7 8 9; do
    ./test_cylinder_wetting A60 L${level}
done
```
**Expected**: First-order convergence in radius error

### Figure 11-12 (Section 4.1): Mass Absorption
**Test**: All tests with mass conservation monitoring
**Expected**: Mass absorption 1-5%, first-order convergence with Δ

### Figure 15-17 (Section 4.2): Sessile Droplet
**Test**: `test_sliding_droplet.c`
**Reproduce**:
```bash
# Horizontal plane - various angles
for angle in 15 30 60 90 120 150 165; do
    ./test_sliding_droplet A${angle} P0 L8
done

# Inclined 45° plane
for angle in 15 30 60 90 120 150 165; do
    ./test_sliding_droplet A${angle} P45 L8
done
```
**Expected**:
- Horizontal: Good agreement all angles
- 45° inclined: Pinning issues for 15° and 165°

### Figure 19 (Section 4.2.2): Gravity Effect
**Test**: `test_sliding_droplet.c`
**Reproduce**:
```bash
# Vary Eötvös number
for eo in 0.001 0.01 0.1 1 10 50; do
    ./test_sliding_droplet A60 P0 E${eo} L8
done
```
**Expected**: Transition from spherical cap (Eo<<1) to puddle (Eo>>1)

---

## Comparison with Paper

### Analysis Tools

1. **compare_with_paper.py**:
   - Loads reference data from paper
   - Compares with simulation results
   - Calculates error metrics
   - Generates pass/fail report

2. **plot_comparisons.py**:
   - Recreates paper figures
   - Overlays simulation results
   - Publication-quality plots
   - Side-by-side comparisons

3. **generate_paper_report.py**:
   - Comprehensive validation report
   - All test cases
   - Statistical analysis
   - LaTeX/PDF output

### Reference Data Sources

1. **Paper figures**: Digitized using WebPlotDigitizer
2. **Experimental data**: From paper tables
3. **Theoretical solutions**: Analytical expressions
4. **Other simulations**: Basilisk reference results

---

## Validation Criteria

### Overall Suite Pass

✅ **PASS** if:
- ≥ 80% of individual tests pass
- All critical tests (1-3) pass
- No major discrepancies with paper
- Error < 5% for quantitative metrics

⚠️ **WARNING** if:
- 60-80% of tests pass
- Some qualitative agreement issues
- Error 5-10% for some metrics

❌ **FAIL** if:
- < 60% of tests pass
- Critical tests fail
- Major discrepancies with paper
- Error > 10% systematically

### Per-Test Criteria

Each test has specific metrics defined in test file and `validation_criteria.md`.

---

## Key Differences: Current Implementation vs Paper

### Similarities ✅

- **VOF method**: Both use geometric VOF
- **Embedded boundaries**: Both support complex geometries
- **Contact angles**: Dynamic contact line models
- **Grid**: Adaptive mesh refinement
- **Two-phase**: Incompressible Navier-Stokes

### Implementation Details

| Aspect | Paper (Tavares 2024) | Our Implementation |
|--------|---------------------|-------------------|
| **VOF Advection** | Weymouth-Yue (2010) | Sharp conservative VOF (Huang 2025) |
| **Contact Angle** | Afkhami-Bussmann | Embed contact with hysteresis |
| **Curvature** | Height functions | Height functions |
| **Grid** | Quadtree/Octree | Quadtree/Octree (Basilisk) |
| **Solver** | Projection method | Centered Navier-Stokes |
| **Embed** | Cut-cell method | Basilisk embed |

### Expected Performance

- **Mass conservation**: Both should achieve < 1e-10 error
- **Contact angles**: Both should match within 5°
- **Spreading**: Both should match correlations within 5-10%
- **Stability**: Both should handle We, Re up to O(1000)

---

## Implementation Notes

### From Paper Section 3.6-3.7

**Ghost Cell Method**:
- Contact angle BC imposed by setting volume fraction F in solid cells (C=0)
- Weighted averaging for cells neighboring triple point
- Works with both static and dynamic contact angles

**Mass Conservation**:
- Solid fraction ignored in mixed cells (0<F<1, 0<C<1)
- Leads to ~1-5% "mass absorption" (first-order in Δ)
- Solid acts as "slightly porous" at scale < Δ

**Contact Line Dynamics**:
- Numerical slip inherent in VOF advection
- Optional: Navier slip BC for mesh-independent results
- No-slip shows mesh-dependent spreading rate

**Grid Alignment**:
- Best results when embedded boundary aligns with grid
- Poor alignment (e.g., 45° plane) can cause pinning
- Extreme angles (θ<30°, θ>150°) more sensitive

---

## Known Issues & Limitations

From paper discussion:

1. **Contact Line Pinning** (Section 4.2):
   - Occurs on poorly aligned grids (45° plane)
   - Worse for extreme angles (15°, 165°)
   - Due to zero VOF flux in certain configurations

2. **Mass Absorption** (Section 4.1, 4.2):
   - 1-5% typical, depending on angle
   - First-order convergence with grid refinement
   - Could be improved with flux correction in mixed cells

3. **Shallow Angles** (Conclusion):
   - θ < 10° or θ > 170° may need special treatment
   - Not addressed in current implementation

4. **Grid Convergence** (Section 4.4):
   - No-slip BC: mesh-dependent contact line velocity
   - Navier slip BC: convergence requires resolving slip length (λ>4Δ)

---

## Performance

Typical runtimes (on standard workstation):

| Test | Grid | Cells | Time | Memory |
|------|------|-------|------|--------|
| Sliding droplet | L=8 | ~64² | ~5 min | ~100 MB |
| Capillary rise | L=9 | ~512² | ~15 min | ~500 MB |
| Cylinder wetting | L=8 | ~256² | ~20 min | ~200 MB |
| Film flow | L=8 | ~256² | ~10 min | ~200 MB |

---

## Prerequisites

### Software

- Basilisk C (compiled, qcc in PATH)
- Sharp VOF headers (in repository)
- Python 3 with numpy, matplotlib, scipy
- (Optional) WebPlotDigitizer for extracting paper data

### Reference Data

Place digitized data from paper in `reference_data/`:
- Figure 3 → `paper_fig3_contact_angle.txt`
- Figure 5 → `paper_fig5_sliding.txt`
- Figure 7 → `paper_fig7_impact.txt`
- etc.

Format: tab or space-separated columns with headers

---

## Usage Example

```bash
# Navigate to paper validation directory
cd paper_validation_2402.10185

# Run all tests (quick mode for development)
./run_paper_tests.sh --quick

# Check results
cat results/paper_validation_summary.json

# Generate plots comparing with paper
cd analysis
python3 plot_comparisons.py --all --save

# Generate LaTeX report
python3 generate_paper_report.py --output validation_report.pdf
```

---

## Expected Timeline

- **Quick tests** (~15-30 min): Basic validation, lower resolution
- **Paper quality** (~2-4 hours): Full resolution matching paper
- **Complete suite** (~8 hours): All cases + analysis + report

---

## Contributing

To add new test cases from the paper:

1. Create `test_cases/test_new_case.c`
2. Document in this README
3. Add reference data to `reference_data/`
4. Update `run_paper_tests.sh`
5. Add comparison in `analysis/compare_with_paper.py`

---

## References

1. **Primary Paper**:
   Tavares, M., Josserand, C., Limare, A., Lopez-Herrera, J.M., and Popinet, S. (2024).
   "A coupled VOF/embedded boundary method to model two-phase flows on arbitrary solid surfaces"
   arXiv:2402.10185v1 [physics.flu-dyn]

2. **Basilisk Documentation**:
   http://basilisk.fr/

3. **Related Methods**:
   - Popinet, S. (2009). "An accurate adaptive solver for surface-tension-driven interfacial flows"
     *J. Comput. Phys.*, 228:5838-5866
   - Afkhami, S., Zaleski, S., and Bussmann, M. (2009). "A mesh-dependent model for applying dynamic contact angles to VOF simulations"
     *J. Comput. Phys.*, 228(15):5370-5389

---

## Citation

If you use this test suite, please cite:

```bibtex
@article{tavares2024coupled,
  title={A coupled VOF/embedded boundary method to model two-phase flows on arbitrary solid surfaces},
  author={Tavares, Mathilde and Josserand, Christophe and Limare, Alexandre and Lopez-Herrera, Jos{\'e} Mar{\'\i}a and Popinet, St{\'ephane}},
  journal={Journal of Computational Physics},
  volume={},
  pages={113975},
  year={2025},
  doi={10.1016/j.jcp.2025.113975}
}
```

---

## Contact

For questions about this test suite:
- Create an issue in the repository
- Check Basilisk forums: http://basilisk.fr/Forum/

---

**Last Updated**: 2025-01-17
**Version**: 1.0
**Status**: Complete Implementation
