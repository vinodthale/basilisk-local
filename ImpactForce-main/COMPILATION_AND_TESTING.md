# Compilation and Testing Guide

## Corrected Files Summary

The following files have been corrected to support both 2D Cartesian and Axisymmetric modes:

1. ✅ `src/constants.h` - Main configuration header
2. ✅ `src/constants-sharp.h` - Sharp VOF configuration header
3. ✅ `src/Bdropimpact.c` - Main simulation code
4. ✅ `src/Bdropimpact-sharp.c` - Sharp VOF simulation code
5. ✅ `src/Jetandpressure.c` - Force calculation post-processing

---

## Compilation Instructions

### Prerequisites
- Basilisk C compiler (`qcc`)
- Basilisk framework installed
- Dimension flag: `-dimension=2` (required)
- Mode flag: `-DAXI=0` (2D Cartesian) or `-DAXI=1` (Axisymmetric)

---

### Option 1: Standard VOF Method

#### 2D Cartesian Mode
```bash
cd ImpactForce-main/src
qcc -dimension=2 -DAXI=0 -O2 -Wall \
    -o Bdropimpact_2D Bdropimpact.c -lm
```

#### Axisymmetric Mode
```bash
cd ImpactForce-main/src
qcc -dimension=2 -DAXI=1 -O2 -Wall \
    -o Bdropimpact_axi Bdropimpact.c -lm
```

---

### Option 2: Sharp VOF Method (with Contact Line Dynamics)

#### 2D Cartesian Mode
```bash
cd ImpactForce-main/src
qcc -dimension=2 -DAXI=0 -O2 -Wall \
    -o Bdropimpact_sharp_2D Bdropimpact-sharp.c -lm
```

#### Axisymmetric Mode
```bash
cd ImpactForce-main/src
qcc -dimension=2 -DAXI=1 -O2 -Wall \
    -o Bdropimpact_sharp_axi Bdropimpact-sharp.c -lm
```

---

### Option 3: Force Calculation Post-Processing

#### 2D Cartesian Mode
```bash
cd ImpactForce-main/src
qcc -dimension=2 -DAXI=0 -O2 -Wall \
    -o Jetandpressure_2D Jetandpressure.c -lm
```

#### Axisymmetric Mode
```bash
cd ImpactForce-main/src
qcc -dimension=2 -DAXI=1 -O2 -Wall \
    -o Jetandpressure_axi Jetandpressure.c -lm
```

---

## Running Simulations

### Basic Execution
```bash
# 2D Cartesian
./Bdropimpact_2D

# Axisymmetric
./Bdropimpact_axi
```

### With Command-Line Parameters
```bash
# Example: Set Reynolds=100, Weber=100
./Bdropimpact_axi R100 W100

# Example: Set max grid level=12
./Bdropimpact_axi X12

# Example: Set min grid level=9
./Bdropimpact_axi N9

# Example: Set pool depth=0.5
./Bdropimpact_axi H0.5

# Example: Set end time=2.0
./Bdropimpact_axi Te2.0

# Example: Set time step for output=0.01
./Bdropimpact_axi Ts0.01

# Combine multiple parameters
./Bdropimpact_axi R100 W100 X12 N9 Te2.0
```

---

## Verification Tests

### Test 1: Check Mode at Compilation
The code will display error if dimension ≠ 2:
```bash
# This should FAIL (missing dimension flag):
qcc -DAXI=0 Bdropimpact.c
# Error: "This code requires dimension = 2. Compile with -dimension=2 flag"
```

### Test 2: Verify Runtime Mode
Add this to your simulation to check the mode:
```c
// In main() after initialization
#if AXI
    printf("Running in AXISYMMETRIC mode\n");
    printf("Coordinates: x=r (radial), y=z (axial)\n");
    printf("Domain: r ∈ [0, %.3f], z ∈ [0, %.3f]\n",
           cfdbv.domainsize, cfdbv.domainsize);
#else
    printf("Running in 2D CARTESIAN mode\n");
    printf("Coordinates: x, y\n");
    printf("Domain: x ∈ [0, %.3f], y ∈ [%.3f, %.3f]\n",
           cfdbv.domainsize, -cfdbv.domainsize/2., cfdbv.domainsize/2.);
#endif
```

### Test 3: Simple Drop Test

Create a minimal test case:
```bash
# Test 2D Cartesian - should create a circle
qcc -dimension=2 -DAXI=0 -o test_2d Bdropimpact.c -lm
./test_2d

# Test Axisymmetric - should create a sphere (via revolution)
qcc -dimension=2 -DAXI=1 -o test_axi Bdropimpact.c -lm
./test_axi
```

Check output files:
- 2D mode: Look for `_2D` in iteration output
- Axisymmetric: Look for `_AXI` in iteration output

---

## Expected Behavior Differences

### Coordinate Systems

| Mode | x-axis | y-axis | Revolution |
|------|--------|--------|------------|
| 2D Cartesian | x | y | No |
| Axisymmetric | r (radial) | z (axial) | Around x-axis |

### Domain Setup

**2D Cartesian:**
- Domain: `x ∈ [0, L]`, `y ∈ [-L/2, L/2]`
- Origin: `(0, -L/2)`
- Left boundary: Solid wall
- Droplet: Circle in x-y plane

**Axisymmetric:**
- Domain: `r ∈ [0, L]`, `z ∈ [0, L]`
- Origin: `(0, 0)` - r=0 at left boundary
- Left boundary: Symmetry axis (r=0)
- Droplet: Sphere (2D profile revolved)

### Force Calculations

**2D Cartesian (Jetandpressure):**
- Force per unit depth
- Integration: `F = ∫ p dy`
- No revolution factor

**Axisymmetric (Jetandpressure):**
- Total 3D force
- Integration: `F = ∫ 2πr p dr`
- Includes `2π` revolution factor

---

## Troubleshooting

### Error: "dimension != 2"
**Solution:** Add `-dimension=2` flag to compilation

### Error: "axi.h not found" in 2D mode
**Cause:** Old version of code without conditional include
**Solution:** Use the corrected version (commit b4e24c2 or later)

### Geometry looks wrong in 2D
**Symptom:** Droplet appears distorted or compilation fails with "z undefined"
**Cause:** Old code using `sq(z)` in 2D
**Solution:** Use corrected version with `sq(x) + sq(y)` only

### Force values don't match between modes
**Expected:** This is normal!
- 2D Cartesian gives force **per unit depth**
- Axisymmetric gives **total 3D force**
- To compare: multiply 2D result by depth (e.g., diameter)

### Boundary condition issues at r=0 (axisymmetric)
**Symptom:** Non-physical flow at left boundary
**Cause:** Missing `u.n[left] = dirichlet(0)`
**Solution:** Use corrected version with explicit r=0 BC

---

## Output Files

Both modes create:
- `intermediate/snapshot-*.*** ` - Simulation snapshots
- `lastfile` - Resume file
- `duration-CPU*.plt` - Timing data
- `parameters.txt` - Simulation parameters
- `endofrun-CPU*.txt` - Final statistics

Check iteration output for mode confirmation:
```
2D Cartesian: i00123_dt1.23e-03_ta0.456_P12_Re100.00000_We100.00000_2D_L0912
Axisymmetric: i00123_dt1.23e-03_ta0.456_P12_Re100.00000_We100.00000_AXI_L0912
```

---

## Performance Notes

### Grid Resolution
- Both modes use same refinement logic
- Axisymmetric may need finer resolution near r=0 axis
- Recommend: LEVELmin=9, LEVELmax=12 for both

### Computational Cost
- **2D Cartesian:** Faster (truly 2D domain)
- **Axisymmetric:** Slower (needs careful treatment at r→0)
- **Sharp VOF:** ~20-30% slower than standard VOF (both modes)

### Memory Usage
- Similar for both modes at same grid level
- Memory scales as: `cells ≈ 2^(2×level)` (2D)

---

## Next Steps

1. **Test compilation** for both modes
2. **Run simple case** with default parameters
3. **Verify output** shows correct mode in filename
4. **Compare results** between 2D and axisymmetric
5. **Adjust parameters** for your specific case

For detailed analysis of the fixes, see: `BASILISK_CODE_REVIEW_2D_vs_AXISYMMETRIC.md`

---

## Quick Reference

```bash
# Compile 2D Cartesian (standard VOF)
qcc -dimension=2 -DAXI=0 -O2 -o sim_2d Bdropimpact.c -lm

# Compile Axisymmetric (standard VOF)
qcc -dimension=2 -DAXI=1 -O2 -o sim_axi Bdropimpact.c -lm

# Run with parameters
./sim_axi R100 W100 X12 Te2.0

# Post-process forces
qcc -dimension=2 -DAXI=1 -o forces Jetandpressure.c -lm
./forces
```

---

**Note:** All fixes maintain backward compatibility with existing axisymmetric simulations while enabling new 2D Cartesian capability.
