# Basilisk Code Review: 2D Cartesian vs Axisymmetric

**Date**: 2025-11-18
**Repository**: ImpactForce-main
**Files Analyzed**:
- `src/constants.h`
- `src/constants-sharp.h`
- `src/Bdropimpact.c`
- `src/Bdropimpact-sharp.c`
- `src/Jetandpressure.c`

---

## CRITICAL ISSUES FOUND

### ❌ ISSUE 1: `axi.h` Always Included - BREAKS 2D Cartesian Mode

**Location**: `constants.h:5`, `constants-sharp.h:8`

**Problem**:
```c
// constants.h line 5
#include "axi.h"                       // axisymmetric geometry
```

The `axi.h` header is **unconditionally included** regardless of whether axisymmetric mode is enabled. This forces axisymmetric coordinate transformations even when compiling for 2D Cartesian.

**Affects**:
- ❌ **2D Cartesian**: BROKEN - Forces cylindrical coordinates when planar is needed
- ✅ **Axisymmetric**: CORRECT - Enables cylindrical coordinates

**Explanation**:
- In Basilisk, including `axi.h` transforms coordinates to cylindrical (r, z, θ)
- For 2D Cartesian, you need standard Cartesian coordinates (x, y)
- The current code cannot run in true 2D planar mode because axi.h overrides everything

**Corrected Code**:

```c
// For 2D Cartesian mode (NO axi.h):
#if AXI
  #include "axi.h"                     // axisymmetric geometry
#endif
#include "navier-stokes/centered.h"
```

---

### ❌ ISSUE 2: Missing `dimension = 2` Declaration

**Location**: All source files

**Problem**:
None of the files explicitly declare `dimension = 2` at the beginning.

**Affects**:
- ❌ **Both 2D Cartesian and Axisymmetric**: Missing required declaration

**Explanation**:
Basilisk requires explicit dimension declaration before including solver headers.

**Corrected Code**:

```c
// At the very top of constants.h or constants-sharp.h:
#if dimension != 2
  #error "This code requires dimension = 2"
#endif
```

Or compile with: `qcc -dimension=2 -DAXI=1 Bdropimpact.c`

---

### ❌ ISSUE 3: Origin Setup Wrong for 2D Cartesian

**Location**: `Bdropimpact.c:26-30`, `Bdropimpact-sharp.c:34-38`

**Problem**:
```c
size(cfdbv.domainsize);
#if AXI
	;  // No origin set - defaults to (0, 0)
#else
	origin(0, -cfdbv.domainsize / 2., -cfdbv.domainsize / 2.);
#endif
```

**Issues**:
1. **2D Cartesian**: Sets origin with THREE coordinates `(0, -L/2, -L/2)` but 2D only has TWO coordinates!
2. **Axisymmetric**: Correctly defaults to `(0, 0)` meaning r=0 at left boundary

**Affects**:
- ❌ **2D Cartesian**: WRONG - Uses 3D origin for 2D problem
- ✅ **Axisymmetric**: CORRECT - r=0 at left boundary

**Explanation**:
- **2D Cartesian**: Domain should be `(x, y) ∈ [0, L] × [-L/2, L/2]`
- **Axisymmetric**: Domain should be `(r, z) ∈ [0, L] × [0, L]` with r=0 at left
- The `origin(0, -L/2, -L/2)` assumes 3D where you center y and z

**Corrected Code**:

```c
size(cfdbv.domainsize);
#if AXI
	// Axisymmetric: r ∈ [0, L], z ∈ [0, L]
	// Default origin (0, 0) is correct: r=0 at left boundary
	;
#else
	#if dimension == 3
		// 3D Cartesian: center y and z
		origin(0, -cfdbv.domainsize / 2., -cfdbv.domainsize / 2.);
	#elif dimension == 2
		// 2D Cartesian: center y only
		origin(0, -cfdbv.domainsize / 2.);
	#endif
#endif
```

---

### ❌ ISSUE 4: Geometry Initialization - Wrong Formula for 2D Cartesian

**Location**:
- `Bdropimpact.c:57` (initfraction event)
- `Bdropimpact.c:74-75, 79, 85` (init event)
- Similar in `Bdropimpact-sharp.c`

**Problem**:
```c
// Line 57 - initfraction
fraction(f,(min(sq(0.50*cfdbv.diameter)-(sq(x - x0) + sq(y) + sq(z)),
               -(sq(0.50*(cfdbv.bubblediameter*cfdbv.diameter))
                -(sq(x - Bubtx0) + sq(y) + sq(z))))));

// Line 79 - init event
if(sq(x - x0) + sq(y) + sq(z) < sq(0.50*cfdbv.diameter))
```

**Affects**:
- ❌ **2D Cartesian**: WRONG - Uses `sq(z)` but z doesn't exist in 2D!
- ✅ **Axisymmetric**: CORRECT - Creates sphere of revolution
- ✅ **3D Cartesian**: CORRECT - Creates sphere

**Explanation**:

| Mode | Coordinates | Sphere Formula | Notes |
|------|-------------|----------------|-------|
| 2D Cartesian | (x, y) | `sq(x-x0) + sq(y-y0) < R²` | Circle in x-y plane |
| Axisymmetric | (r, z) → (x, y) in code | `sq(x-x0) + sq(y) < R²` | Rotated around x-axis → sphere |
| 3D Cartesian | (x, y, z) | `sq(x-x0) + sq(y-y0) + sq(z-z0) < R²` | Full 3D sphere |

The current code uses `sq(x - x0) + sq(y) + sq(z)`:
- In **axisymmetric**, this creates: `sq(r - r0) + sq(z - z0) < R²` → revolves to sphere ✅
- In **2D Cartesian**, `z` doesn't exist → COMPILATION ERROR ❌

**Corrected Code**:

```c
event initfraction (t = 0)
{
	double x0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;
	double Bubtx0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;

	#if AXI
		// Axisymmetric: creates sphere when revolved around x-axis
		// Use (x,y) = (r,z) coordinates
		fraction(f,(min(
			sq(0.50*cfdbv.diameter)-(sq(x - x0) + sq(y)),
			-(sq(0.50*(cfdbv.bubblediameter*cfdbv.diameter))-(sq(x - Bubtx0) + sq(y)))
		)));
	#else
		// 2D Cartesian: creates circle in x-y plane
		fraction(f,(min(
			sq(0.50*cfdbv.diameter)-(sq(x - x0) + sq(y)),
			-(sq(0.50*(cfdbv.bubblediameter*cfdbv.diameter))-(sq(x - Bubtx0) + sq(y)))
		)));
	#endif
}
```

**For init event (line 76-91)**:

```c
foreach ()
{
	f[] = 0.0;

	#if AXI
		// Axisymmetric: sphere geometry
		if(sq(x - x0) + sq(y) < sq(0.50*cfdbv.diameter))
	#else
		// 2D Cartesian: circle geometry
		if(sq(x - x0) + sq(y) < sq(0.50*cfdbv.diameter))
	#endif
	{
		f[] = 1.0;
		u.x[] = -cfdbv.vel;
		u.y[] = 0.0;
	}

	#if AXI
		// Bubble in axisymmetric
		if(sq(x - Bubtx0) + sq(y) < sq(0.50*(cfdbv.bubblediameter * cfdbv.diameter)))
	#else
		// Bubble in 2D Cartesian
		if(sq(x - Bubtx0) + sq(y) < sq(0.50*(cfdbv.bubblediameter * cfdbv.diameter)))
	#endif
	{
		f[] = 0.0;
		u.x[] = -cfdbv.vel;
		u.y[] = 0.0;
	}
};
```

**Note**: The formula is actually the SAME for both modes in this case! The difference is:
- **Axisymmetric**: (x,y) represents (r,z), revolution creates 3D sphere
- **2D Cartesian**: (x,y) represents (x,y), creates 2D circle

The original code with `sq(z)` would only work in 3D mode.

---

### ❌ ISSUE 5: Missing Axisymmetric Boundary Condition at r=0

**Location**: `Bdropimpact.c:9-15`, `Bdropimpact-sharp.c:16-22`

**Problem**:
```c
u.t[left] = dirichlet(0);  // No slip at surface
f[left] = 0.;              // non wetting
u.n[right] = neumann(0);   // Free flow condition
p[right] = dirichlet(0);   // 0 pressure far from surface
u.n[top] = neumann(0);     // Allows outflow through boundary
p[top] = dirichlet(0);     // 0 pressure far from surface
// Default for bottom is symmetry
```

**Issues**:

For **Axisymmetric** mode:
- `left` boundary = r = 0 axis (symmetry axis)
- ❌ MISSING: `u.n[left] = dirichlet(0)` - radial velocity must be zero at r=0
- ✅ `u.t[left] = dirichlet(0)` is correct (tangential velocity = 0)
- ⚠️ `f[left] = 0` means non-wetting, which may not be appropriate for symmetry axis

For **2D Cartesian** mode:
- `left` boundary = solid wall
- ✅ `u.t[left] = dirichlet(0)` is correct (no-slip)
- ✅ `f[left] = 0` is correct (non-wetting wall)
- ❌ MISSING: Should also set `u.n[left] = dirichlet(0)` for no-penetration

**Affects**:
- ❌ **Axisymmetric**: Missing critical symmetry condition at r=0
- ❌ **2D Cartesian**: Missing no-penetration at wall

**Explanation**:

**Axisymmetric r=0 Boundary** (symmetry axis):
- Radial velocity: `u_r(r=0) = 0` (no flow through axis)
- Axial velocity: `∂u_z/∂r|_{r=0} = 0` (symmetry)
- VOF field: `∂f/∂r|_{r=0} = 0` (symmetry)
- The `axi.h` solver automatically handles some of this, but explicit BC is safer

**2D Cartesian Left Boundary** (solid wall):
- Normal velocity: `u_n = 0` (no penetration)
- Tangential velocity: `u_t = 0` (no slip)
- VOF: `f = 0` (non-wetting) or `f = 1` (wetting)

**Corrected Code**:

```c
// Boundary conditions
#if AXI
	// Axisymmetric: left = r=0 symmetry axis
	u.n[left] = dirichlet(0);    // No radial flow through axis
	u.t[left] = dirichlet(0);    // Tangential velocity = 0 at axis
	// f[left]: use neumann(0) for symmetry, not dirichlet(0)
	// But axi.h may handle this automatically

	// Right boundary: far field
	u.n[right] = neumann(0);
	p[right] = dirichlet(0);

	// Top boundary: far field or outflow
	u.n[top] = neumann(0);
	p[top] = dirichlet(0);

	// Bottom: symmetry (if z=0 is symmetry plane)
	// Default is fine
#else
	// 2D Cartesian: left = solid wall
	u.n[left] = dirichlet(0);    // No penetration
	u.t[left] = dirichlet(0);    // No slip
	f[left] = 0.;                // Non-wetting wall

	// Right boundary: far field
	u.n[right] = neumann(0);
	p[right] = dirichlet(0);

	// Top boundary: far field or outflow
	u.n[top] = neumann(0);
	p[top] = dirichlet(0);

	// Bottom: symmetry or far field
	u.n[bottom] = neumann(0);
	p[bottom] = dirichlet(0);
#endif
```

**Important Note**:
The `axi.h` module in Basilisk automatically enforces `u.x[left] = 0` (radial velocity at r=0) and metric singularity handling. However, explicit boundary conditions are clearer and avoid potential issues.

---

### ❌ ISSUE 6: Force Calculation - Axisymmetric Only

**Location**: `Jetandpressure.c:1, 49-60`

**Problem**:
```c
// Line 1
#include "axi.h"

// Lines 49-52: Reference pressure at top boundary
foreach_boundary(top){
    pdatum += 2*pi*y*pressure[]*(Delta);
    wt += 2*pi*y*(Delta);
}

// Lines 56-60: Force on left boundary
foreach_boundary(left){
    pForce += 2*pi*y*(Delta)*(pressure[]-pdatum);
    pleft += pressure[];
}
```

**Affects**:
- ✅ **Axisymmetric**: CORRECT - Integrates with cylindrical weighting `2πr`
- ❌ **2D Cartesian**: WRONG - Should not use `2πy` factor

**Explanation**:

**Axisymmetric Integration**:
- Surface area element: `dS = 2πr · dz` where r = radial coordinate
- In Basilisk axi.h: x → r, y → z
- Force: `F = ∫∫ p · dS = ∫ 2πy · p(y) · dy` (integration over z-coordinate called 'y')
- This is what the code computes: `2*pi*y*(Delta)*(pressure[]-pdatum)`

**2D Cartesian Integration**:
- Line element: `dL = dy` (no revolution)
- Force per unit depth: `F = ∫ p · dy`
- Should be: `(Delta)*(pressure[]-pdatum)` (no 2π factor)

**Note**: This file `Jetandpressure.c` is **specifically for axisymmetric** post-processing. It includes `axi.h` unconditionally, so it won't work for 2D Cartesian without modification.

**Corrected Code** (to support both modes):

```c
#if AXI
  #include "axi.h"
#endif
#include "navier-stokes/centered.h"
// ... other includes ...

#define VOFFOLDER "FVFVDb0.00delta0.00V0.18"
scalar pressure[];
double cfdbvbubblediameter = 0.00;

#if AXI
  double PreFactor = 2*pi;  // For axisymmetric integration
#else
  double PreFactor = 1.0;    // For 2D Cartesian (force per unit depth)
#endif

// ... in init event ...

foreach_boundary(top){
    #if AXI
        pdatum += 2*pi*y*pressure[]*(Delta);  // Cylindrical
        wt += 2*pi*y*(Delta);
    #else
        pdatum += pressure[]*(Delta);          // Cartesian
        wt += (Delta);
    #endif
}

// ...

foreach_boundary(left){
    #if AXI
        pForce += 2*pi*y*(Delta)*(pressure[]-pdatum);  // Cylindrical
    #else
        pForce += (Delta)*(pressure[]-pdatum);          // Cartesian (per unit depth)
    #endif
    pleft += pressure[];
}
```

---

### ❌ ISSUE 7: Refinement Criteria Use 3D Formula

**Location**: `Bdropimpact.c:74-75`

**Problem**:
```c
refine(sq(x - x0) + sq(y) + sq(z) < sq(0.50*cfdbv.diameter + cfdbv.refinegap)
    && sq(x - x0) + sq(y) + sq(z) > sq(0.50*cfdbv.diameter - cfdbv.refinegap)
    && level < LEVELmax);
```

Uses `sq(z)` which doesn't exist in 2D!

**Corrected Code**: Same as Issue 4 - remove `sq(z)` for 2D.

---

## SUMMARY OF CHANGES REQUIRED

### For 2D CARTESIAN Mode:

1. **Remove axi.h** or conditionally include it
2. **Add `dimension = 2`** at compilation or in code
3. **Fix origin**: Use `origin(0, -L/2)` not `origin(0, -L/2, -L/2)`
4. **Fix geometry**: Remove `sq(z)` from all sphere/circle formulas
5. **Fix refinement**: Remove `sq(z)` from refinement criteria
6. **Fix boundaries**: Ensure proper wall boundary conditions
7. **Fix force calculation**: Remove `2*pi*y` factor in Jetandpressure.c

### For AXISYMMETRIC Mode:

1. **Keep axi.h** included
2. **Add `dimension = 2`** at compilation or in code
3. **Keep origin** at default (0, 0) ✅
4. **Fix geometry**: Remove `sq(z)`, keep `sq(x) + sq(y)` ✅ (mostly correct)
5. **Add r=0 boundary**: Explicitly set `u.n[left] = dirichlet(0)`
6. **Check VOF boundary**: Verify `f[left]` handling at symmetry axis
7. **Force calculation**: Already correct ✅

---

## COMPLETE CORRECTED EXAMPLE

### constants.h (Fixed Version)

```c
// Author: Vinod Thale  15 sep 2023
// Conditional compilation for 2D Cartesian vs Axisymmetric

// Basilisk requires dimension declaration
// Compile with: qcc -dimension=2 -DAXI=0 Bdropimpact.c  (for 2D Cartesian)
// Compile with: qcc -dimension=2 -DAXI=1 Bdropimpact.c  (for Axisymmetric)

#if dimension != 2
  #error "This code requires dimension = 2. Compile with -dimension=2"
#endif

// Only include axi.h for axisymmetric mode
#if AXI
  #include "axi.h"                       // axisymmetric geometry
#endif

#include "navier-stokes/centered.h"      // solve NS equations
#define FILTERED 1                       // Smear density and viscosity jumps
#include "two-phase.h"                   // Solve two-phase equation
#include "tension.h"                     // include surface tension between phases
#include "tag.h"                         // help to count small droplets
#include "curvature.h"

// ... rest of constants.h unchanged ...
```

### Bdropimpact.c (Fixed Version - Key Sections)

```c
#include "constants.h"

// ... global variables ...

// Boundary conditions
#if AXI
	// Axisymmetric: left boundary is r=0 symmetry axis
	u.n[left] = dirichlet(0);    // No radial flow at axis
	u.t[left] = dirichlet(0);    // No tangential velocity at axis
	// VOF symmetry handled by axi.h
#else
	// 2D Cartesian: left boundary is solid wall
	u.n[left] = dirichlet(0);    // No penetration
	u.t[left] = dirichlet(0);    // No slip
	f[left] = 0.;                // Non-wetting
#endif

// Right and top boundaries (same for both)
u.n[right] = neumann(0);
p[right] = dirichlet(0);
u.n[top] = neumann(0);
p[top] = dirichlet(0);

int main(int argc, char **argv)
{
	// ... initialization ...

	size(cfdbv.domainsize);

	#if AXI
		// Axisymmetric: r ∈ [0, L], z ∈ [0, L]
		// Default origin (0, 0) places r=0 at left boundary - CORRECT
		;
	#else
		// 2D Cartesian: x ∈ [0, L], y ∈ [-L/2, L/2]
		origin(0, -cfdbv.domainsize / 2.);
	#endif

	// ... rest of main ...
}

event initfraction (t = 0)
{
	double x0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;
	double Bubtx0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;

	// Both modes use same formula: sq(x-x0) + sq(y-y0) < R²
	// Axisymmetric: (x,y)=(r,z) revolves to create sphere
	// 2D Cartesian: (x,y)=(x,y) creates circle
	fraction(f,(min(
		sq(0.50*cfdbv.diameter)-(sq(x - x0) + sq(y)),
		-(sq(0.50*(cfdbv.bubblediameter*cfdbv.diameter))-(sq(x - Bubtx0) + sq(y)))
	)));
}

event init(i = 0)
{
	if (restore(file = FILENAME_LASTFILE))
	{
		#if AXI
			boundary((scalar *){fm});  // Metric field for axi
		#endif
	}
	else
	{
		double x0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;
		double Bubtx0 = cfdbv.pooldepth + cfdbv.initialdis + cfdbv.diameter*0.50;

		// Refinement along drop interface
		refine(sq(x - x0) + sq(y) < sq(0.50*cfdbv.diameter + cfdbv.refinegap)
			&& sq(x - x0) + sq(y) > sq(0.50*cfdbv.diameter - cfdbv.refinegap)
			&& level < LEVELmax);

		// Refinement along bubble interface
		refine(sq(x - Bubtx0) + sq(y) < sq(0.50*(cfdbv.bubblediameter * cfdbv.diameter) + cfdbv.refinegap)
			&& sq(x - Bubtx0) + sq(y) > sq(0.50*(cfdbv.bubblediameter*cfdbv.diameter) - cfdbv.refinegap)
			&& level < LEVELmax);

		foreach ()
		{
			f[] = 0.0;

			// Drop initialization
			if(sq(x - x0) + sq(y) < sq(0.50*cfdbv.diameter))
			{
				f[] = 1.0;
				u.x[] = -cfdbv.vel;
				u.y[] = 0.0;
			}

			// Bubble initialization
			if(sq(x - Bubtx0) + sq(y) < sq(0.50*(cfdbv.bubblediameter * cfdbv.diameter)))
			{
				f[] = 0.0;
				u.x[] = -cfdbv.vel;
				u.y[] = 0.0;
			}
		};

		// ... rest of init ...
	}
}

// ... rest of events unchanged ...
```

### Jetandpressure.c (Fixed Version)

```c
#if AXI
  #include "axi.h"
#endif
#include "navier-stokes/centered.h"
#define FILTERED 1
#include "two-phase.h"
#include "tension.h"
#include "tag.h"
#include "curvature.h"

#define VOFFOLDER "FVFVDb0.00delta0.00V0.18"
scalar pressure[];
double cfdbvbubblediameter = 0.00;

#if AXI
  double PreFactor = 2*pi;  // For axisymmetric: integrate 2πr·dz
#else
  double PreFactor = 1.0;   // For 2D Cartesian: integrate dy (force per unit depth)
#endif

int main (int argc, char **argv)
{
	run();
}

event init (t = 0)
{
	// ... loop over snapshots ...

	for (timeload = timebgn; timeload <= timeend; timeload += timestp)
	{
		sprintf (namefile, "intermediate/snapshot-%5.4f", timeload);
		restore (file = namefile);

		// Calculate reference pressure at top boundary
		double pdatum = 0, wt = 0;
		foreach_boundary(top){
			#if AXI
				pdatum += 2*pi*y*pressure[]*(Delta);  // Axisymmetric: 2πr
				wt += 2*pi*y*(Delta);
			#else
				pdatum += pressure[]*(Delta);          // 2D Cartesian: linear
				wt += (Delta);
			#endif
		}
		if (wt >0){
			pdatum /= wt;
		}

		// Calculate force on left boundary
		double pForce = 0.;
		double pleft = 0.;
		foreach_boundary(left)
		{
			#if AXI
				pForce += 2*pi*y*(Delta)*(pressure[]-pdatum);  // Axisymmetric
			#else
				pForce += (Delta)*(pressure[]-pdatum);          // 2D Cartesian
			#endif
			pleft += pressure[];
		}

		boundary((scalar *){f, u.x, u.y, pressure});

		fprintf (ForceonLfet, "%f  %.10f %.10f\r\n", timeload, pForce, pleft);
		fclose (ForceonLfet);
		nff++;
	}
}

event end(t = 0.0)
{
	printf("\r\n-------\r\nEND!\r\n");
}
```

---

## COMPILATION INSTRUCTIONS

### For 2D Cartesian Mode:
```bash
qcc -dimension=2 -DAXI=0 -O2 -Wall \
    -o Bdropimpact_2D Bdropimpact.c -lm
```

### For Axisymmetric Mode:
```bash
qcc -dimension=2 -DAXI=1 -O2 -Wall \
    -o Bdropimpact_axi Bdropimpact.c -lm
```

### Verify Mode at Runtime:
Add to `main()`:
```c
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

---

## PHYSICS DIFFERENCES

### 2D Cartesian:
- **Geometry**: Infinite cylinder (circle extruded in z)
- **Volume**: Area per unit depth
- **Forces**: Force per unit depth
- **Droplet**: Infinite cylindrical droplet
- **Physics**: Planar flow, no curvature in z-direction

### Axisymmetric:
- **Geometry**: Sphere (2D profile revolved around axis)
- **Volume**: Full 3D volume
- **Forces**: Total 3D force
- **Droplet**: Spherical droplet
- **Physics**: 3D effects from revolution, curvature in θ-direction

---

## ADDITIONAL RECOMMENDATIONS

1. **Add runtime checks**: Verify dimension and AXI flag consistency
2. **Document coordinate system**: Add comments explaining (r,z) vs (x,y)
3. **Separate header files**: Create `constants-2d.h` and `constants-axi.h`
4. **Unit tests**: Create simple test cases for each mode
5. **Visualization**: Ensure output distinguishes between modes
6. **Contact angle**: In axisymmetric, verify contact line at r=0 axis
7. **Adaptive refinement**: Check metric terms don't cause issues at r→0

---

## CRITICAL TAKEAWAYS

1. ✅ **DO**: Conditionally include `axi.h` only for axisymmetric
2. ✅ **DO**: Remove `sq(z)` terms - they don't exist in 2D
3. ✅ **DO**: Set correct origin for each mode
4. ✅ **DO**: Use `2*pi*y` integration factor only in axisymmetric
5. ✅ **DO**: Enforce r=0 symmetry conditions in axisymmetric
6. ❌ **DON'T**: Mix 3D formulas in 2D code
7. ❌ **DON'T**: Assume axi.h handles everything automatically

The current code is **primarily designed for axisymmetric** with broken 2D Cartesian support due to unconditional `axi.h` inclusion and 3D geometric formulas.
