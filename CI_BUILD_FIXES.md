# CI Build Fixes Report

## Summary
This document details the issues encountered in the GitHub Actions CI workflow for the Basilisk VOF Method compilation and the solutions implemented to fix them.

---

## Issue #1: YAML Syntax Error (Line 80-81)

### Error
```
Invalid workflow file: .github/workflows/ci-compile.yml#L80
You have an error in your yaml syntax on line 80
```

### Root Cause
The workflow used a bash heredoc (`<<'EOF'`) inside a YAML multiline string. GitHub Actions YAML parser couldn't correctly interpret the unindented heredoc content, causing syntax errors.

### Original Code (Problematic)
```yaml
- name: Create qcc wrapper & export environment
  run: |
    set -euo pipefail
    cat > "$BASILISK_ROOT/qcc-wrapper.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
BASILISK_BIN="${BASILISK:-/home/runner/basilisk/src}"
exec "$BASILISK_BIN/qcc" "$@"
EOF
```

### Solution
Replaced heredoc with `printf` to avoid YAML parsing ambiguity:

```yaml
- name: Create qcc wrapper & export environment
  run: |
    set -euo pipefail
    printf '#!/usr/bin/env bash\nset -euo pipefail\nBASILISK_BIN="${BASILISK:-/home/runner/basilisk/src}"\nexec "$BASILISK_BIN/qcc" "$@"\n' > "$BASILISK_ROOT/qcc-wrapper.sh"
```

**Status:** ✅ Fixed

---

## Issue #2: Missing ast/libast.a Library

### Error
```
make: *** No rule to make target 'ast/libast.a', needed by 'qcc'. Stop.
Error: Process completed with exit code 2.
```

### Root Cause
The `qcc` compiler build depends on `ast/libast.a` (line 28 in `src/Makefile`):
```makefile
qcc: qcc.c include.o postproc.o ast/libast.a config ast/ast.h
```

The workflow attempted to build `qcc` directly without first building the required `ast` library subdirectory.

### Original Code (Problematic)
```yaml
echo "Building qcc (ONLY)..."
make -j1 qcc
```

### Solution
Added explicit build step for the ast library before building qcc:

```yaml
echo "Building ast library..."
make -C ast

echo "Building qcc..."
make -j1 qcc
```

**Status:** ✅ Fixed

---

## Issue #3: Missing Graphics Libraries (glutils, fb_tiny)

### Error
```
/usr/bin/ld: cannot find -lglutils: No such file or directory
/usr/bin/ld: cannot find -lfb_tiny: No such file or directory
collect2: error: ld returned 1 exit status
[ERROR] ✗ Compilation reported success but binary not created: circle-droplet.c
```

### Root Cause
The VOF method simulations require graphics libraries (`libglutils.a` and `libfb_tiny.a`) for visualization support. These libraries are built in the `src/gl` subdirectory but were not being compiled during the CI build process.

### Solution
Added build step for gl libraries:

```yaml
echo "Building ast library..."
make -C ast

echo "Building gl libraries (glutils, fb_tiny)..."
make -C gl

echo "Building qcc..."
make -j1 qcc
```

**Status:** ✅ Fixed

---

## Final Working Build Sequence

The correct build order for Basilisk in CI is:

1. **Build ast library** (`make -C ast`)
   - Required for qcc compiler

2. **Build gl libraries** (`make -C gl`)
   - Provides libglutils.a and libfb_tiny.a
   - Required for VOF simulations with graphics support

3. **Build qcc compiler** (`make -j1 qcc`)
   - Depends on ast/libast.a
   - Core Basilisk compiler

---

## Pull Requests

- **PR #34**: Initial CI workflow corrections (path standardization)
- **PR #35**: YAML heredoc syntax fix attempt
- **PR #36**: Alternative YAML syntax fix
- **PR #37**: qcc build dependency fix
- **PR #38**: Complete fix (ast + gl libraries + YAML syntax)

---

## Lessons Learned

1. **YAML Heredocs**: Bash heredocs inside YAML multiline strings can cause parsing issues. Use `printf` or alternative methods for generating multi-line file content.

2. **Makefile Dependencies**: Always check Makefile dependencies before attempting partial builds. The `qcc` target explicitly lists `ast/libast.a` as a prerequisite.

3. **Library Dependencies**: Graphics-enabled simulations require the gl libraries even in "minimal" builds. The linker flags `-lglutils` and `-lfb_tiny` must have corresponding `.a` files.

4. **Build Order Matters**: In hierarchical build systems, subdirectories with library outputs must be built before targets that link against them.

---

## Final Configuration

### Environment Variables
```yaml
env:
  BASILISK_ROOT: /home/runner/basilisk
  BASILISK_SRC: /home/runner/basilisk/src
```

### Build Steps
```yaml
- Build system dependencies (apt packages)
- Cache Basilisk installation
- Build ast library (make -C ast)
- Build gl libraries (make -C gl)
- Build qcc compiler (make -j1 qcc)
- Create qcc wrapper script
- Export environment variables
- Compile VOF simulations
- Collect and upload artifacts
```

---

## Testing

The CI workflow successfully:
- ✅ Builds qcc compiler
- ✅ Compiles VOF method simulations
- ✅ Links against required graphics libraries
- ✅ Passes YAML validation
- ✅ Caches Basilisk installation for faster subsequent runs

---

**Date:** 2025-11-21
**Branch:** `claude/fix-build-ast-and-yaml-014NLXeJpxW2FCZ13P7Ju1Ji`
**Status:** Ready for merge to main
