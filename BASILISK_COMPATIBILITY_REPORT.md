# Basilisk Compatibility Report
## Wetting Test Suite Analysis

**Date**: 2025-11-17
**Branch**: `claude/implement-wetting-tests-013H21MBpE1abvXkvA3MjaLQ`
**Test Directory**: `ImpactForce-main/tests/paper_validation_2402.10185/`

---

## Executive Summary

⚠️ **CRITICAL COMPATIBILITY ISSUES FOUND**

The newly implemented wetting test suite has **incompatibilities with standard Basilisk** that will prevent compilation. The tests use a `contact_angle[]` scalar field approach that requires a specialized Sharp VOF implementation, but they are currently importing the standard `contact.h` header.

**Status**: ❌ **Will NOT compile with standard Basilisk**
**Fix Required**: Update includes to use Sharp VOF embedded contact angle implementation

---

## Detailed Analysis

### 1. Contact Angle Implementation Mismatch

#### **Issue**: Wrong `contact.h` Implementation

The test files use:
```c
#include "contact.h"  // Standard Basilisk - WRONG!

// Later in code:
foreach() {
    contact_angle[] = theta_rad;  // Treats contact_angle as scalar field
}
```

**Standard Basilisk (`src/contact.h`):**
- `contact_angle()` is a **MACRO** for boundary conditions
- Only works on flat boundaries (top/bottom/left/right)
- Requires height function vectors: `h.t[bottom] = contact_angle(theta)`
- **Does NOT support embedded boundaries**
- **Does NOT define `contact_angle` as a scalar field**

**What the tests need:**
```c
#include "embed_contact.h"  // Sharp VOF implementation

// This implementation defines:
scalar contact_angle[], * interfaces_contact_angle = {contact_angle};
```

**Reference**: Found in `/2D-sharp-and-conservative-VOF-method-Basiliks-main/embed_contact.h`

This is based on:
- Paper: "A sharp VOF method for modeling 2D contact line moving" (Huang et al.)
- Located in: `2D-sharp-and-conservative-VOF-method-Basiliks-main/`
- Implements contact angles for **embedded boundaries** using **ghost cell method**

---

### 2. Grid Dimension Issues

#### **Issue**: 2D Grid with 3D Coordinate References

**File**: `test_capillary_rise.c`
- Line 51: `#include "grid/quadtree.h"` (2D grid)
- Line 116: `double r = sqrt(sq(x) + sq(z));` ❌ **`z` does not exist in 2D!**
- Line 186: `double r = sqrt(sq(x) + sq(z));` ❌

**File**: `test_cylinder_wetting.c`
- Line 50: `#include "grid/quadtree.h"` (2D grid)
- Line 193: `fraction(f, -sqrt(sq(x) + sq(y - y_center) + sq(z)) + R0);` ❌

**In Basilisk:**
- `quadtree` → 2D grid → Only `x`, `y` coordinates
- `octree` → 3D grid → `x`, `y`, `z` coordinates
- Using `z` in a `quadtree` simulation will cause **compilation errors**

**Fix**: Either:
1. Change to 2D coordinates (remove `sq(z)`)
2. Switch to `octree` grid for 3D

---

### 3. Missing Header Dependencies

The tests are importing standard headers but need Sharp VOF versions:

| Current (WRONG) | Should Use (CORRECT) |
|----------------|---------------------|
| `#include "contact.h"` | `#include "embed_contact.h"` |
| `#include "tension.h"` | `#include "embed_tension.h"` (possibly) |
| `#include "two-phase.h"` | `#include "embed_two-phase.h"` (possibly) |

Additionally, the Sharp VOF implementation may require:
- `#include "tmp_fraction_field.h"`
- `#include "embed_correct_height.h"`
- `#include "embed_heights.h"`
- `#include "embed_vof.h"`

---

## File-by-File Issues

### ❌ `test_capillary_rise.c`

**Issues**:
1. Line 56: Uses `contact.h` instead of `embed_contact.h`
2. Line 116, 186: References `z` coordinate in 2D grid
3. Line 197: Sets `contact_angle[]` (undefined scalar)

**Severity**: HIGH - Will not compile

---

### ❌ `test_cylinder_wetting.c`

**Issues**:
1. Line 55: Uses `contact.h` instead of `embed_contact.h`
2. Line 193: References `z` coordinate in 2D grid
3. Line 198: Sets `contact_angle[]` (undefined scalar)

**Severity**: HIGH - Will not compile

---

### ❌ `test_sliding_droplet.c`

**Issues**:
1. Line 48: Uses `contact.h` instead of `embed_contact.h`
2. Line 235: Sets `contact_angle[]` (undefined scalar)

**Severity**: HIGH - Will not compile

---

### ❌ `test_film_flow.c`

**Issues**:
1. Line 53: Uses `contact.h` instead of `embed_contact.h`
2. Line 210: Sets `contact_angle[]` (undefined scalar)

**Severity**: HIGH - Will not compile

---

## Comparison with Working Examples

### Standard Basilisk Example (`src/test/sessile.c`)
```c
#include "grid/multigrid.h"
#include "navier-stokes/centered.h"
#include "contact.h"
#include "vof.h"
#include "tension.h"

scalar f[], * interfaces = {f};
vector h[];  // Height function vector

// Set contact angle on flat boundary
h.t[bottom] = contact_angle (theta0*pi/180.);
f.height = h;  // Associate with VOF tracer
```

**Key differences:**
- Uses height function **vector** `h[]`
- Sets boundary condition, not scalar field
- Only works for flat boundaries

---

### Sharp VOF Implementation Pattern

Based on `2D-sharp-and-conservative-VOF-method-Basiliks-main/circle-droplet.c`:
```c
#include "myembed.h"
#include "navier-stokes/centered.h"
#include "embed_two-phase.h"
#include "embed_tension.h"
#include "embed_vof.h"
#include "embed_contact.h"

// contact_angle[] is now defined as a scalar field
foreach() {
    contact_angle[] = theta * pi / 180.0;  // This works!
}
```

---

## Recommendations

### Immediate Fixes Required

1. **Update all test files to use Sharp VOF headers:**

```c
// Replace this:
#include "grid/quadtree.h"
#include "embed.h"
#include "navier-stokes/centered.h"
#include "two-phase.h"
#include "tension.h"
#include "contact.h"

// With this:
#include "grid/quadtree.h"
#include "myembed.h"              // Sharp VOF embedded boundary
#include "navier-stokes/centered.h"
#include "embed_two-phase.h"       // Sharp VOF two-phase
#include "embed_tension.h"         // Sharp VOF tension
#include "embed_vof.h"             // Sharp VOF advection
#include "embed_contact.h"         // Sharp VOF contact angles
```

2. **Fix coordinate system issues:**
   - `test_capillary_rise.c`: Remove all `sq(z)` references
   - `test_cylinder_wetting.c`: Remove `sq(z)` from line 193

3. **Update include paths:**
   Add path to Sharp VOF headers when compiling:
   ```bash
   qcc -O2 -I../../../2D-sharp-and-conservative-VOF-method-Basiliks-main \
       test_sliding_droplet.c -o test_sliding_droplet -lm
   ```

4. **Verify scalar field initialization:**
   Ensure `contact_angle[]` is properly initialized as part of interfaces

---

## Testing Plan

After fixes are applied:

1. **Compilation test:**
```bash
cd ImpactForce-main/tests/paper_validation_2402.10185/test_cases
qcc -I../../../../2D-sharp-and-conservative-VOF-method-Basiliks-main \
    test_sliding_droplet.c -o test_sliding_droplet -lm
```

2. **Run validation:**
```bash
./test_sliding_droplet A60 P0 L8
```

3. **Check for errors:**
   - Compilation errors → Include path issues
   - Runtime segfaults → Scalar field not initialized
   - Wrong physics → Contact angle implementation issues

---

## Additional Notes

### Paper Reference

The tests are based on:
**"A coupled VOF/embedded boundary method to model two-phase flows on arbitrary solid surfaces"**
Tavares et al., arXiv:2402.10185 (2024)

This paper describes a **ghost cell method** for contact angles on embedded boundaries, which is what the Sharp VOF implementation provides.

### Sharp VOF Reference

The required implementation is based on:
**"A sharp VOF method for modeling 2D contact line moving"**
Huang et al.

Available in: `2D-sharp-and-conservative-VOF-method-Basiliks-main/`

---

## Conclusion

**Current Status**: ❌ **NOT Compatible with Standard Basilisk**

The tests will **fail to compile** with standard Basilisk because:
1. `contact_angle[]` scalar field is undefined
2. Coordinate system mismatches (2D grid with 3D coordinates)
3. Missing Sharp VOF implementation headers

**Recommended Action**:
1. Update all includes to use Sharp VOF headers
2. Fix coordinate system issues
3. Update compilation instructions in README
4. Test compilation and execution

**Estimated Fix Time**: 1-2 hours

---

## References

- Standard Basilisk: `/home/user/basilisk-local/src/contact.h`
- Sharp VOF Implementation: `/home/user/basilisk-local/2D-sharp-and-conservative-VOF-method-Basiliks-main/`
- Example sessile drop: `/home/user/basilisk-local/src/test/sessile.c`
- Test suite: `/home/user/basilisk-local/ImpactForce-main/tests/paper_validation_2402.10185/`
