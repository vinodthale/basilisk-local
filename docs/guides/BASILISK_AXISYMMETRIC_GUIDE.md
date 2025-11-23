# Basilisk Axisymmetric Coordinate System Reference Guide

## Quick Reference: `axi.h` Coordinate System

### **CRITICAL: Coordinate Convention in `axi.h`**

```
In Basilisk's axi.h:
┌─────────────────────────────────────────┐
│  x = AXIAL direction (vertical/z)       │
│  y = RADIAL direction (horizontal/r)    │
│  y ≥ 0 (always non-negative)            │
│  Axis of symmetry at y = 0              │
└─────────────────────────────────────────┘
```

### Visual Representation

```
     y (radial) →

 x   ╔════════════════════════════╗
 ↓   ║                            ║
(a   ║    Computational Domain    ║
 x   ║       (half-plane)         ║
 i   ║                            ║
 a   ╚════════════════════════════╝
 l)  ← Axis of symmetry (y = 0)
```

The domain is **rotated around the y = 0 line** to create the full 3D geometry.

---

## Common Mistakes and Corrections

### ❌ **WRONG** (Common Mistake)
```c
// INCORRECT coordinate interpretation
double r = x;          // ❌ x is NOT radial
double z = y;          // ❌ y is NOT axial
```

### ✅ **CORRECT**
```c
// CORRECT coordinate interpretation
double z_axial = x;    // ✓ x is axial (vertical)
double r_radial = y;   // ✓ y is radial (horizontal)
```

---

## Detailed Examples

### 1. Sphere/Droplet Initialization

#### ❌ WRONG (swapped coordinates)
```c
// This creates a weird shape, NOT a sphere!
fraction(f, sq(R) - sq(x) - sq(y - y0));
```

#### ✅ CORRECT
```c
// Sphere centered at x = x0, on axis (y = 0)
// Equation: r² + (x - x0)² < R²
fraction(f, sq(R) - sq(y) - sq(x - x0));
```

**Explanation:** In axisymmetric, `y` represents the radial distance from axis, so sphere equation uses `sq(y)` for the radial term.

### 2. Gravity Direction

For a droplet **falling vertically downward**:

#### ❌ WRONG
```c
const face vector g[] = {0., -9.8};  // ❌ Gravity in -y (radial)
```

#### ✅ CORRECT
```c
const face vector g[] = {-9.8, 0.};  // ✓ Gravity in -x (axial/vertical)
```

### 3. Embedded Geometry (Horizontal Plate)

For a **horizontal plate** perpendicular to the fall direction:

#### ❌ WRONG
```c
// Trying to create plate at constant y (radial)
if (y >= plate_pos && y <= plate_pos + thickness) {
    // Solid region
}
```

#### ✅ CORRECT
```c
// Horizontal plate at constant x (axial position)
if (x >= plate_pos && x <= plate_pos + thickness) {
    // Check radial extent
    if (y > orifice_radius) {
        // Solid region
    }
}
```

### 4. Interface Tracking on Axis

To find interface position **on the centerline**:

#### ❌ WRONG
```c
foreach() {
    if (x < 0.01 && f[] > 0.5) {  // ❌ x ≈ 0 is NOT the axis
        // Track position
    }
}
```

#### ✅ CORRECT
```c
foreach() {
    if (y < 0.01 && f[] > 0.5) {  // ✓ y ≈ 0 is the axis
        // Track axial position: x
    }
}
```

### 5. Volume Integration

Axisymmetric volumes require a **2π factor**.

#### ❌ WRONG (missing factor)
```c
double volume = 0.;
foreach(reduction(+:volume)) {
    volume += f[] * dv();  // ❌ Missing axisymmetric factor
}
```

#### ✅ CORRECT
```c
double volume = 0.;
foreach(reduction(+:volume)) {
    // Axisymmetric: dV = 2π * r * dx * dr
    volume += f[] * 2.0 * pi * y * dv();  // ✓ Multiply by 2π*y
}
```

---

## Boundary Conditions in `axi.h`

```c
// Boundary naming convention:
u.n[left]   // x = 0 (axial: inlet/bottom)
u.n[right]  // x = L0 (axial: outlet/top)
u.n[bottom] // y = 0 (AXIS - automatic symmetry)
u.n[top]    // y = L0 (far-field radial)
```

**Important:** The boundary at `y = 0` (bottom) is the **axis of symmetry**. Basilisk handles this automatically - you typically **don't need to specify** boundary conditions there.

### Example: Flow Along Axis

```c
// Inflow at left (axial direction)
u.n[left] = dirichlet(U_inlet);
u.t[left] = dirichlet(0.);

// Outflow at right (axial direction)
u.n[right] = neumann(0.);
p[right] = dirichlet(0.);

// Far-field radial boundary
u.n[top] = dirichlet(0.);     // No penetration
u.t[top] = neumann(0.);       // Free slip

// y = 0 (axis): NO BC NEEDED - automatic symmetry
```

---

## Comparison Table: 2D Cartesian vs Axisymmetric

| Feature | 2D Cartesian | 2D Axisymmetric (`axi.h`) |
|---------|--------------|---------------------------|
| **Header** | Default (or `grid/cartesian.h`) | `#include "axi.h"` |
| **x coordinate** | Horizontal (arbitrary) | **Axial** (vertical/streamwise) |
| **y coordinate** | Vertical (arbitrary) | **Radial** (r ≥ 0) |
| **Symmetry axis** | None | **y = 0** (automatic) |
| **Origin** | Any | `origin(0., 0.)` to ensure y ≥ 0 |
| **Sphere equation** | x² + y² < R² | y² + (x-x₀)² < R² |
| **Volume element** | dV = dx·dy | dV = 2π·y·dx·dy |
| **Gravity (vertical)** | {0., -g} | {-g, 0.} |
| **Coordinates for on-axis** | Any valid (x,y) | **y = 0** |
| **Plate (horizontal)** | Constant y | Constant **x** |

---

## Checklist for Converting Code

### From 2D Cartesian to Axisymmetric

- [ ] Add `#include "axi.h"` before solver
- [ ] Add `origin(0., 0.)` in `main()`
- [ ] Swap understanding: **x ↔ z (axial), y ↔ r (radial)**
- [ ] Update sphere/circle equations: use `sq(y)` for radial term
- [ ] Rotate gravity: vertical fall = `-x` direction
- [ ] Fix embedded boundaries: horizontal surfaces = constant `x`
- [ ] Update volume calculations: multiply by `2π·y`
- [ ] Fix tracking: on-axis means `y ≈ 0`
- [ ] Review boundary conditions: no BC needed at `y = 0`
- [ ] Protect divisions: check `y > ε` before dividing by radius

### From Axisymmetric to 2D Cartesian

- [ ] Remove `#include "axi.h"`
- [ ] Remove axisymmetric volume factors (`2π·y`)
- [ ] Treat coordinates as standard (x, y) in plane
- [ ] Update sphere equations to standard form
- [ ] Adjust gravity to desired direction
- [ ] Update embedded boundaries for planar geometry
- [ ] Change boundary conditions (no automatic symmetry)

---

## Common Pitfalls

### 1. **Coordinate Confusion**
**Problem:** Assuming x = radial, y = axial (opposite of truth)

**Symptoms:**
- Spheres appear as weird ellipsoids
- Gravity pulls in wrong direction
- Plates oriented incorrectly
- Volume conservation fails

**Fix:** Use correct mapping (x=axial, y=radial) everywhere

### 2. **Missing Volume Factor**
**Problem:** Forgetting `2π·y` in integration

**Symptoms:**
- Volume conservation shows incorrect values
- Forces/energies off by large factor

**Fix:** Always multiply by `2π·y` when integrating

### 3. **Division by Zero at Axis**
**Problem:** Computing `something/y` at `y = 0`

**Symptoms:**
- NaN values
- Simulation crashes
- Unphysical results near axis

**Fix:** Protect with `y > ε` check:
```c
double y_safe = (y > 1e-10) ? y : 1e-10;
double result = something / y_safe;
```

### 4. **Wrong Boundary at Axis**
**Problem:** Setting boundary conditions at `y = 0`

**Symptoms:**
- Symmetry broken
- Flow patterns incorrect
- May cause solver issues

**Fix:** Remove BC at `y = 0` - it's automatic

### 5. **Incorrect Geometry Orientation**
**Problem:** Plate defined at constant `y` instead of constant `x`

**Symptoms:**
- Geometry appears as vertical wall instead of horizontal plate
- Orifice geometry incorrect

**Fix:** Use constant `x` for horizontal surfaces

---

## Quick Debug Tips

### How to Check if Coordinates are Correct

1. **Print coordinate ranges:**
```c
event init(t = 0) {
    double x_min = HUGE, x_max = -HUGE;
    double y_min = HUGE, y_max = -HUGE;

    foreach() {
        if (x < x_min) x_min = x;
        if (x > x_max) x_max = x;
        if (y < y_min) y_min = y;
        if (y > y_max) y_max = y;
    }

    fprintf(stderr, "Domain: x ∈ [%g, %g], y ∈ [%g, %g]\n",
            x_min, x_max, y_min, y_max);

    // For axi.h: expect y_min ≈ 0
    if (y_min < -1e-10) {
        fprintf(stderr, "WARNING: y < 0 detected! Not proper axisymmetric.\n");
    }
}
```

2. **Visualize initial geometry:**
```c
event init(t = 0) {
    // Output initial droplet shape
    FILE *fp = fopen("initial_shape.dat", "w");
    fprintf(fp, "# x(axial) y(radial) f\n");
    foreach() {
        if (f[] > 0.01)
            fprintf(fp, "%g %g %g\n", x, y, f[]);
    }
    fclose(fp);
}
```

Plot with gnuplot or Python to verify shape makes sense.

3. **Check sphere volume:**
```c
// After initializing sphere with radius R
double vol = 0.;
foreach(reduction(+:vol)) {
    vol += f[] * 2.0 * pi * y * dv();
}
double expected = 4./3. * pi * pow(R, 3);
fprintf(stderr, "Volume: computed = %g, expected = %g, error = %.2f%%\n",
        vol, expected, 100. * fabs(vol - expected) / expected);
```

For a correctly initialized sphere, error should be < 1%.

---

## Reference: Basilisk `axi.h` Source Comments

From `src/axi.h`:
```c
/**
# Axisymmetric coordinates

This is a "metric" which implements axisymmetric coordinates with
the origin of the rotation axis at (x,y) = (0,0). The "radial"
coordinate is *y*.
*/
```

**Key takeaway:** "The radial coordinate is y"

---

## Example Simulations

### Complete Axisymmetric Droplet Example

```c
#include "axi.h"
#include "navier-stokes/centered.h"
#include "two-phase.h"
#include "tension.h"

// Parameters
#define R_DROP 0.001    // 1 mm radius
#define U_IMPACT 1.0    // 1 m/s
#define SIGMA 0.072     // N/m
#define L0_SIZE 0.01    // 10 mm domain

int main() {
    L0 = L0_SIZE;
    origin(0., 0.);  // Axis at y = 0
    init_grid(128);

    // Properties
    rho1 = 1000.;  // Water
    rho2 = 1.2;    // Air
    mu1 = 1e-3;
    mu2 = 1.8e-5;
    f.sigma = SIGMA;

    // Gravity in -x (downward)
    const face vector g[] = {-9.8, 0.};
    a = g;

    run();
}

event init(t = 0) {
    // Droplet centered at x = 0.005, on axis
    // With initial velocity U_IMPACT in -x direction
    fraction(f, sq(R_DROP) - sq(y) - sq(x - 0.005));

    foreach() {
        u.x[] = -U_IMPACT * f[];  // Initial velocity in -x
    }
}

event logfile(i++) {
    // Compute volume with axisymmetric factor
    double vol = 0.;
    foreach(reduction(+:vol)) {
        vol += f[] * 2.0 * pi * y * dv();
    }

    double vol_sphere = 4./3. * pi * pow(R_DROP, 3);
    fprintf(stderr, "t = %g, V/V0 = %g\n", t, vol/vol_sphere);
}
```

---

## Additional Resources

- **Basilisk Documentation:** http://basilisk.fr/
- **Source Code:** `$BASILISK/src/axi.h`
- **Examples:** `$BASILISK/src/examples/`
  - `atomisation.c` - Good axisymmetric example
  - `bubble.c` - Rising bubble in axisymmetric

---

## Summary

**Remember the golden rule for `axi.h`:**

> **x = axial (vertical), y = radial (horizontal, y ≥ 0), axis at y = 0**

When in doubt:
1. Check the source: `src/axi.h`
2. Test with simple sphere: volume should be (4/3)πR³
3. Visualize: does the geometry make physical sense?
4. Compare with working examples: `Bdropimpact.c` in this repository

---

**Document Version:** 1.0
**Last Updated:** 2025-11-18
**Repository:** https://github.com/vinodthale/basilisk-local
