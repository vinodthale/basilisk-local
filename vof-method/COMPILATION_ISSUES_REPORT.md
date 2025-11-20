# VOF-METHOD COMPILATION ISSUES ANALYSIS

**Analysis Date:** 2025-11-20  
**Status:** CRITICAL ISSUES FOUND  
**Project:** Basilisk VOF Method Implementation  

---

## EXECUTIVE SUMMARY

The analysis of all `.c` and `.h` files in the `/home/user/basilisk-local/vof-method/` directory identified **5 CRITICAL ISSUES** and several **WARNINGS** that will prevent successful compilation.

### Critical Issues to Fix:
1. **Function Name Mismatch** - `cleansmallcell()` called but `fractions_cleanup()` defined
2. **Missing Header File** - `adapt_wavelet_limited.h` does not exist
3. **Type Mismatch** - `contact_angle` declared as vector but may need to be scalar
4. **Undefined Function** - `init_grid()` availability uncertain in some configurations
5. **Incomplete Implementation** - `polygon_fraction()` uses simplified algorithm

---

## CRITICAL ISSUES

### 1. FUNCTION NAME MISMATCH: cleansmallcell() vs fractions_cleanup()

**Severity:** CRITICAL - Will cause COMPILATION ERROR  
**Type:** Undefined Function Reference

#### Files Affected:
- `/home/user/basilisk-local/vof-method/circle-droplet.c` (Line 89)
- `/home/user/basilisk-local/vof-method/droplet-impact-orifice.c` (Line 179)
- `/home/user/basilisk-local/vof-method/droplet-impact-orifice-nondim.c` (Line 210)

#### Issue Description:
The code calls `cleansmallcell(cs, fs, csTL)` but the actual function defined in `myembed.h` (Line 291) is called `fractions_cleanup()`.

#### Current Code:
```c
cleansmallcell (cs, fs, csTL);  // UNDEFINED FUNCTION!
```

#### Actual Function Definition in myembed.h (Line 291):
```c
int fractions_cleanup (scalar c, face vector s,
           double smin = 0., bool opposite = false)
```

#### Fix:
Replace all calls to `cleansmallcell()` with `fractions_cleanup()`:
```c
// Change from:
cleansmallcell (cs, fs, csTL);

// To:
fractions_cleanup (cs, fs, 1e-2, false);
```

Note: The function signature expects `smin` (minimum surface fraction) as third parameter instead of `csTL`. Adjust accordingly:
```c
fractions_cleanup (cs, fs, max(1e-2, VFTL), false);
```

---

### 2. MISSING HEADER FILE: adapt_wavelet_limited.h

**Severity:** CRITICAL - Will cause COMPILATION ERROR  
**Type:** Missing Include File

#### Files Affected:
- `/home/user/basilisk-local/vof-method/droplet-impact-round-orifice.c` (Line 38)
- `/home/user/basilisk-local/vof-method/droplet-impact-sharp-orifice.c` (Line 41)
- `/home/user/basilisk-local/vof-method/droplet-impact-sharp-orifice-nondim.c` (Line 53)

#### Issue Description:
The header file `adapt_wavelet_limited.h` is included but does not exist in either:
- The vof-method directory
- The standard Basilisk library (`/home/user/basilisk-local/src/`)

#### Current Code:
```c
#include "adapt_wavelet_limited.h"
...
adapt_wavelet_limited ((scalar *){f, u.x, u.y},
                       (double[]){0.01, 0.01, 0.01},
                       MAXLEVEL, MINLEVEL);
```

#### Fix:
Replace with standard Basilisk `adapt_wavelet`:

```c
// Change from:
#include "adapt_wavelet_limited.h"
adapt_wavelet_limited ((scalar *){f, u.x, u.y},
                       (double[]){0.01, 0.01, 0.01},
                       MAXLEVEL, MINLEVEL);

// To:
#include "adapt_wavelet.h"
adapt_wavelet ((scalar *){f, u.x, u.y},
               (double[]){0.01, 0.01, 0.01},
               MAXLEVEL, MINLEVEL);
```

---

### 3. TYPE MISMATCH: contact_angle Declaration

**Severity:** HIGH - May cause RUNTIME ERROR or INCORRECT BEHAVIOR  
**Type:** Type Mismatch / Field Attribute Conflict

#### Files Affected:
- `/home/user/basilisk-local/vof-method/droplet-impact-sharp-orifice.c` (Lines 93-94, 118-119)
- `/home/user/basilisk-local/vof-method/droplet-impact-sharp-orifice-nondim.c` (Lines 108-109, 134-135)

#### Issue Description:
`contact_angle` is declared as a `vector` field, but Basilisk's standard `contact.h` expects a `scalar` field. Declaring it as a vector allows per-dimension storage but may conflict with the contact angle module's expectations.

#### Current Code (droplet-impact-sharp-orifice.c):
```c
vector contact_angle[];  // Line 93 - Declared as vector

...

f.contact_angle = contact_angle;  // Line 118
...
contact_angle.x[] = THETA_PINNING;  // Line 194
contact_angle.y[] = THETA_PINNING;  // Line 195
```

#### Standard Basilisk (expected):
```c
scalar contact_angle[];  // Standard: scalar field
f.contact_angle = contact_angle;
contact_angle[] = theta_value;
```

#### Fix Options:

**Option 1: Use scalar (recommended for standard Basilisk)**
```c
scalar contact_angle[];

foreach() {
  contact_angle[] = THETA_ADVANCING;  // Single value per cell
}
```

**Option 2: If vector storage is intentional**
Ensure contact.h can handle vector fields, or implement custom contact angle handling that respects the vector structure.

---

### 4. UNDEFINED FUNCTION: init_grid()

**Severity:** MEDIUM - May cause LINKER ERROR depending on configuration  
**Type:** Potentially Undefined Function

#### Files Affected:
- `/home/user/basilisk-local/vof-method/droplet-impact-round-orifice.c` (Line 91)
- `/home/user/basilisk-local/vof-method/droplet-impact-sharp-orifice.c` (Line 101)
- `/home/user/basilisk-local/vof-method/droplet-impact-sharp-orifice-nondim.c` (Line 117)
- `/home/user/basilisk-local/vof-method/droplet-impact-sharp-orifice-nondim-CORRECTED.c` (Line 126)

#### Current Code:
```c
int main() {
  size (L0);
  origin (0., 0.);
  init_grid (1 << MINLEVEL);  // Line 91, 101, 117, 126
  ...
}
```

#### Issue Description:
`init_grid()` is typically provided by `navier-stokes/centered.h` (via `run.h`), but explicit calls in `main()` indicate potential issues:
1. If `navier-stokes/centered.h` is not properly included, `init_grid()` will be undefined
2. Explicit `init_grid()` before `run()` might interfere with Basilisk's internal initialization

#### Expected Usage in Basilisk:
Usually, `run()` calls `init_grid()` internally, so explicit calls are not needed.

#### Fix:
Option 1: Remove explicit `init_grid()` call and let `run()` handle it:
```c
int main() {
  size (L0);
  origin (0., 0.);
  
  rho1 = RHO_DROPLET;
  rho2 = RHO_AMBIENT;
  // ... other initializations ...
  
  run();  // init_grid() is called internally
  return 0;
}
```

Option 2: If explicit grid initialization is needed, ensure proper includes:
```c
#include "run.h"  // Explicit include for init_grid
```

---

### 5. INCOMPLETE IMPLEMENTATION: polygon_fraction()

**Severity:** MEDIUM - Will produce INCORRECT RESULTS in some cases  
**Type:** Simplified/Incomplete Algorithm

#### File Affected:
- `/home/user/basilisk-local/vof-method/tmp_fraction_field.h` (Lines 252-303)

#### Issue Description:
The `polygon_fraction()` function (lines 252-303) is marked as using a SIMPLIFIED ALGORITHM:

```c
// Line 277-278:
"This is a simplified version - full implementation would use
 exact polygon intersection algorithms"
```

The function attempts to compute VOF flux through a polygon swept by advection, but uses:
1. Simple rectangular approximation as fallback
2. Trapezoidal integration with averaged interface positions
3. No exact polygon clipping

#### Current Code Structure (simplified):
```c
double polygon_fraction(coord pp[5], double un, double s, coord p_mof[2],
                       coord mc, double alphac)
{
  if (fabs(un) < 1e-12) {
    return 0.;
  }

  // Count valid polygon vertices
  int n = 0;
  for (int i = 0; i < 5; i++) {
    if (pp[i].x < 9.) {  // Valid vertices have x,y < 10
      n++;
    }
  }

  if (n < 3) {
    // Fall back to rectangle_fraction
    coord n_plane = {mc.x, mc.y, 0.};
    return rectangle_fraction(n_plane, alphac,
                             (coord){-0.5, -0.5, -0.5},
                             (coord){s*un - 0.5, 0.5, 0.5});
  }

  // Simplified flux computation using trapezoidal integration
  double flux = 0.;
  if (p_mof[0].x < 9. && p_mof[1].x < 9.) {
    double y_avg = (p_mof[0].y + p_mof[1].y) / 2.;
    double x_avg = (p_mof[0].x + p_mof[1].x) / 2.;
    if (s > 0) {
      flux = un * (1. + y_avg) / 2.;
    } else {
      flux = un * (1. - y_avg) / 2.;
    }
  }
  else {
    // Fall back to PLIC reconstruction
    coord n_plane = {mc.x, mc.y, 0.};
    flux = rectangle_fraction(n_plane, alphac,
                             (coord){-0.5, -0.5, -0.5},
                             (coord){s*un - 0.5, 0.5, 0.5});
  }

  return flux;
}
```

#### Potential Issues:
1. **Accuracy Loss**: Simplified algorithm may not conserve mass exactly
2. **Edge Cases**: Behavior undefined for complex polygon shapes
3. **MOF Reconstruction**: Expected MOF method not fully implemented

#### Recommendation:
For accurate VOF advection with contact angles:

**Option 1: Use simpler PLIC (Piecewise Linear Interface Reconstruction)**
Replace with standard Basilisk VOF advection that doesn't require `polygon_fraction()`

**Option 2: Implement full MOF (Moment Of Fluid) reconstruction**
This requires more sophisticated polygon clipping algorithms:
```c
// Requires implementing:
// - Polygon-polygon intersection
// - Moment integral computation
// - Full second-order interface reconstruction
```

**Option 3: Add warning/validation**
```c
#ifdef DEBUG
  fprintf(stderr, "WARNING: Using simplified polygon_fraction()\n");
  fprintf(stderr, "  Consider using full MOF reconstruction for accuracy\n");
#endif
```

---

## HIGH PRIORITY WARNINGS

### 1. Function Definition Status: cleansmallcell vs fractions_cleanup

**What's Called:** `cleansmallcell(cs, fs, csTL)`  
**What's Defined:** `fractions_cleanup(c, s, smin, opposite)`

The parameter mapping is NOT straightforward:
- Called with: `(cs, fs, csTL)` - 3 parameters
- Defined as: `(scalar c, face vector s, double smin = 0., bool opposite = false)` - 2-4 parameters

**Fix:** Use the correct function with proper parameters:
```c
fractions_cleanup(cs, fs, max(1e-2, VFTL), false);
```

---

### 2. VFTL and SEPS Variables (Status: OK with caveat)

**Files:** circle-droplet.c (Line 28), droplet-impact-orifice.c (Line 53), droplet-impact-orifice-nondim.c (Line 83)

**Code:**
```c
double csTL = max(1e-2, VFTL);
```

**Status:** PROPERLY DEFINED  
**Location:** `/home/user/basilisk-local/vof-method/embed_height_normal.h`
```c
#define VFTL 1e-10  (Line 318)
#define SEPS 1e-30  (Line 315)
```

**Note:** These are correctly defined and used, but `embed_height_normal.h` must be included (which happens via `myembed.h`).

---

### 3. Functions in tmp_fraction_field.h - Dependency Check

#### Functions Used:
| Function | Location | Status |
|----------|----------|--------|
| `mycs()` | Line 104, 198 | OK - in `fractions.h` |
| `facet_normal()` | Line 79 | OK - in `fractions.h` |
| `plane_alpha()` | Lines 84, 203 | OK - in `fractions.h` |
| `rectangle_fraction()` | Lines 270, 297 | OK - in `geometry.h` (via `fractions.h`) |
| `plane_area_center()` | Line 680 (myembed.h) | OK - in `fractions.h` |

**Conclusion:** All VOF reconstruction functions are properly defined in Basilisk's `fractions.h` and `geometry.h`, which are included in the header chain.

---

### 4. Contact Angle Field Handling

**Files:** droplet-impact-sharp-orifice.c, droplet-impact-sharp-orifice-nondim.c, droplet-impact-sharp-orifice-nondim-CORRECTED.c

**Issue:** The code implements custom contact angle hysteresis and pinning logic:

```c
event contact_angle_update (i++) {
  foreach() {
    if (cs[] > 0 && cs[] < 1) {  // At solid boundary
      if (edge_marker[] > 0.5) {
        // At sharp edge - enforce pinning angle
        contact_angle[] = THETA_PINNING;
      } else {
        // Away from edge - apply hysteresis
        double current_angle = contact_angle[];
        if (current_angle < THETA_RECEDING) {
          contact_angle[] = THETA_RECEDING;
        } else if (current_angle > THETA_ADVANCING) {
          contact_angle[] = THETA_ADVANCING;
        }
      }
    }
  }
  boundary({contact_angle});
}
```

**Concern:** The timing of this event relative to the VOF advection is critical. Ensure it runs BEFORE the main Basilisk advection steps.

---

### 5. Axisymmetric Coordinate System Handling

**Files:** 
- `droplet-impact-orifice.c` - Uses standard coordinates (x=horizontal, y=vertical)
- `droplet-impact-orifice-nondim.c` - Uses standard coordinates
- `droplet-impact-round-orifice.c` - Uses axisymmetric (x=radial, y=vertical) - **CHECK DOCUMENTATION**
- `droplet-impact-sharp-orifice.c` - Uses axisymmetric (x=radial, y=vertical)
- `droplet-impact-sharp-orifice-nondim.c` - Uses axisymmetric (x=radial, y=vertical)
- `droplet-impact-sharp-orifice-nondim-CORRECTED.c` - **CORRECTED** to axi.h conventions (x=axial, y=radial)

**Issue:** The CORRECTED version changed the coordinate system interpretation (Line 90-94 comments):
```c
// In axi.h: x = axial (vertical), y = radial
// So for plate position:
#define PLATE_POS_X    (L0 - POOL_HEIGHT)           // Axial position
#define DROPLET_POS_X  (PLATE_POS_X + DROP_RELEASE) // Axial position
```

**Recommendation:** 
- If using `axi.h`, follow the CORRECTED version's coordinate system
- Add clear comments to other files about coordinate system assumptions
- Test axisymmetric simulations carefully

---

## MEDIUM PRIORITY ISSUES

### 1. Edge Marker Field in Sharp Orifice Cases

**Files:** droplet-impact-sharp-orifice.c, droplet-impact-sharp-orifice-nondim.c, droplet-impact-sharp-orifice-nondim-CORRECTED.c

**Issue:** `edge_marker[]` is used to identify regions near sharp edges for contact line pinning:

```c
scalar edge_marker[];  // Declared

foreach() {
  double dist_upper_edge = sqrt(sq(r - R_ORIFICE) + sq(y_coord - plate_top));
  double dist_lower_edge = sqrt(sq(r - R_ORIFICE) + sq(y_coord - plate_bottom));

  if (dist_upper_edge < EDGE_REGION || dist_lower_edge < EDGE_REGION) {
    edge_marker[] = 1.0;  // Near sharp edge
  } else {
    edge_marker[] = 0.0;  // Away from edge
  }
}
```

**Concern:** 
1. `edge_marker` is initialized in `init` event but not updated afterwards
2. As the mesh refines/coarsens, edge markers may become inconsistent
3. Recommend adding refinement rules or periodic updates

---

## SYNTAX ANALYSIS

### Findings:
- ✓ All braces `{}` are properly matched
- ✓ All parentheses `()` are properly matched
- ✓ All semicolons are present (except in preprocessor directives)
- ✓ Basilisk keywords (`event`, `foreach`, `trace`) used correctly
- ✓ Vector and scalar field declarations are proper syntax

**No critical syntax errors found.**

---

## BASILISK IDIOM COMPLIANCE

### Correct Usage:
- ✓ `event` blocks with conditions (e.g., `event init(t=0)`)
- ✓ `foreach()` loops with optional `reduction()`
- ✓ `boundary()` calls after field modifications
- ✓ `trace` keyword for function profiling
- ✓ Face vectors declared as `face vector name[]`
- ✓ Scalar attributes assigned (e.g., `f.sigma = sigma0`)

### Potential Idiom Issues:
- ? Custom `contact_angle_update` event - ensure it integrates properly with solver
- ? Explicit `init_grid()` call - usually handled by `run()`
- ? Field assignments in `main()` - should be OK but verify scope

---

## SUMMARY TABLE

| Issue | Severity | Type | Files | Fix |
|-------|----------|------|-------|-----|
| cleansmallcell undefined | CRITICAL | Undefined Function | 3 | Rename to `fractions_cleanup()` |
| adapt_wavelet_limited missing | CRITICAL | Missing Header | 3 | Replace with `adapt_wavelet` |
| contact_angle type mismatch | HIGH | Type Error | 3 | Change `vector` to `scalar` |
| init_grid() undefined | MEDIUM | Undefined Function | 4 | Remove or ensure run.h included |
| polygon_fraction simplified | MEDIUM | Incomplete Algorithm | 1 | Consider full MOF reconstruction |
| VFTL/SEPS usage | LOW | ✓ OK | 3 | No action needed |
| VOF functions | LOW | ✓ OK | 1 | No action needed |

---

## RECOMMENDATIONS

### Immediate Actions (Required for Compilation):

1. **Fix function name mismatch:**
   ```bash
   # Replace cleansmallcell with fractions_cleanup
   sed -i 's/cleansmallcell/fractions_cleanup/g' circle-droplet.c droplet-impact-orifice.c droplet-impact-orifice-nondim.c
   ```

2. **Fix missing header:**
   ```bash
   # Replace adapt_wavelet_limited with adapt_wavelet
   sed -i 's/adapt_wavelet_limited/adapt_wavelet/g' droplet-impact-round-orifice.c droplet-impact-sharp-orifice.c droplet-impact-sharp-orifice-nondim.c
   sed -i 's/#include "adapt_wavelet_limited.h"/#include "adapt_wavelet.h"/g' droplet-impact-round-orifice.c droplet-impact-sharp-orifice.c droplet-impact-sharp-orifice-nondim.c
   ```

3. **Fix contact_angle type:**
   ```bash
   # Change vector contact_angle to scalar contact_angle
   sed -i 's/vector contact_angle\[\]/scalar contact_angle[]/g' droplet-impact-sharp-orifice.c droplet-impact-sharp-orifice-nondim.c
   # Also remove .x[] and .y[] accesses, change to []
   ```

### Testing Actions:

1. Test each simulation with corrected files
2. Verify volume conservation in droplet impact simulations
3. Compare results between dimensional and non-dimensional versions
4. Validate contact angle behavior at sharp edges

### Long-term Improvements:

1. Implement full MOF reconstruction in `polygon_fraction()`
2. Add comprehensive test suite for VOF advection
3. Document coordinate system conventions clearly
4. Add validation layer to catch function name mismatches

---

## FILES NEEDING MODIFICATION

### C Files (7 total):

| File | Issues | Priority | Lines to Fix |
|------|--------|----------|--------------|
| circle-droplet.c | cleansmallcell | CRITICAL | 89 |
| droplet-impact-orifice.c | cleansmallcell | CRITICAL | 179 |
| droplet-impact-orifice-nondim.c | cleansmallcell | CRITICAL | 210 |
| droplet-impact-round-orifice.c | adapt_wavelet, init_grid | CRITICAL | 38, 91, 196 |
| droplet-impact-sharp-orifice.c | adapt_wavelet, contact_angle, init_grid | CRITICAL | 41, 93, 101, 118-119 |
| droplet-impact-sharp-orifice-nondim.c | adapt_wavelet, contact_angle, init_grid | CRITICAL | 53, 108-109, 117, 134-135 |
| droplet-impact-sharp-orifice-nondim-CORRECTED.c | contact_angle, init_grid | HIGH | 118, 126, 227-229, 244 |

### Header Files (7 total):

| File | Issues | Priority | Status |
|------|--------|----------|--------|
| myembed.h | None | OK | Ready |
| tmp_fraction_field.h | polygon_fraction simplified | MEDIUM | Consider enhancement |
| embed_height_normal.h | None | OK | Dependency OK |
| embed_*.h (others) | None | OK | Dependencies OK |
| axi.h | None | OK | Ready |
| TPR2D.h | None | OK | Ready |
| embed_curvature.h | Uses mycs() correctly | OK | Ready |

---

## VERIFICATION CHECKLIST

Before compilation:
- [ ] Replace `cleansmallcell()` with `fractions_cleanup()` (3 files)
- [ ] Replace `adapt_wavelet_limited` with `adapt_wavelet` (3 files)
- [ ] Change `vector contact_angle[]` to `scalar contact_angle[]` (3 files)
- [ ] Fix contact_angle element access from `.x[]/.y[]` to `[]`
- [ ] Review `init_grid()` calls - remove or ensure run.h included
- [ ] Test each simulation independently
- [ ] Verify volume conservation
- [ ] Check contact angle behavior at embedded boundaries

---

## CONCLUSION

The VOF-method code has **5 critical issues** that must be fixed before compilation:

1. **Function name mismatch** (cleansmallcell vs fractions_cleanup) - 3 files
2. **Missing header** (adapt_wavelet_limited) - 3 files  
3. **Type mismatch** (contact_angle vector vs scalar) - 3 files
4. **Undefined function** (init_grid) - 4 files
5. **Incomplete algorithm** (polygon_fraction) - 1 file

Additionally, several **medium-priority issues** should be addressed for correctness and robustness. After applying the recommended fixes, all files should compile and run successfully with standard Basilisk.

