#!/bin/bash
#
# CI Compilation and Testing Script for VOF Method
#
# This script compiles all Basilisk C source files in the vof-method directory.
# It follows the proper Basilisk workflow:
#   1. Check for qcc (Basilisk C compiler)
#   2. If available: use qcc to preprocess and compile
#   3. If not available: Report that qcc is needed and exit
#
# Usage:
#   ./ci_run.sh [test]
#
#   test - Run simple test (compile circle-droplet only)
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "VOF Method Compilation Test Script"
echo "========================================="
echo ""

# Function to print status messages
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Change to vof-method directory
cd "$(dirname "$0")"
VOF_DIR=$(pwd)
print_status "Working directory: $VOF_DIR"

# Check for required files
print_status "Checking for required files..."
REQUIRED_FILES=(
    "tmp_fraction_field.h"
    "TPR2D.h"
    "axi.h"
    "myembed.h"
    "embed_contact.h"
    "embed_vof.h"
    "circle-droplet.c"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        print_error "Required file missing: $file"
        exit 1
    fi
done
print_status "All required files present"

# Check for qcc (Basilisk C compiler)
print_status "Checking for Basilisk C compiler (qcc)..."
if command -v qcc >/dev/null 2>&1; then
    print_status "qcc found: $(which qcc)"
    HAS_QCC=true
else
    print_error "qcc not found in PATH"
    print_error "Basilisk C compiler is required to compile these files."
    print_error ""
    print_error "To install Basilisk:"
    print_error "  1. Install dependencies: sudo apt install darcs gcc make gawk"
    print_error "  2. Clone Basilisk: darcs clone http://basilisk.fr/basilisk ~/basilisk"
    print_error "  3. Compile: cd ~/basilisk/src && ln -s config.gcc config && make"
    print_error "  4. Set environment:"
    print_error "     export BASILISK=\$HOME/basilisk"
    print_error "     export PATH=\$PATH:\$BASILISK"
    print_error ""
    print_error "See BASILISK_INSTALL.md for detailed instructions"
    exit 1
fi

# Check BASILISK environment variable
if [ -z "$BASILISK" ]; then
    print_warning "BASILISK environment variable not set"
    print_warning "Trying to auto-detect Basilisk installation..."

    if [ -d "$HOME/basilisk" ]; then
        export BASILISK="$HOME/basilisk"
        print_status "Using BASILISK=$BASILISK"
    elif [ -d "/usr/local/basilisk" ]; then
        export BASILISK="/usr/local/basilisk"
        print_status "Using BASILISK=$BASILISK"
    else
        print_warning "Could not auto-detect BASILISK path"
        print_warning "Proceeding anyway, but compilation may fail"
    fi
else
    print_status "BASILISK=$BASILISK"
fi

echo ""

# Function to compile a single file
compile_file() {
    local source_file=$1
    local binary_name="${source_file%.c}"

    print_status "Compiling $source_file..."

    # Try to compile with qcc
    if qcc -Wall -O2 -o "$binary_name" "$source_file" -lm 2>&1 | tee "${binary_name}.compile.log"; then
        if [ -f "$binary_name" ]; then
            print_status "✓ Successfully compiled $source_file"
            rm -f "${binary_name}.compile.log"
            return 0
        else
            print_error "✗ Compilation reported success but binary not created: $source_file"
            return 1
        fi
    else
        print_error "✗ Failed to compile $source_file"
        if [ -f "${binary_name}.compile.log" ]; then
            print_error "Error log:"
            cat "${binary_name}.compile.log"
        fi
        return 1
    fi
}

# Test mode: compile only circle-droplet.c
if [ "$1" == "test" ]; then
    print_status "Running in TEST mode (circle-droplet only)"
    echo ""
    compile_file "circle-droplet.c"
    RESULT=$?
    echo ""
    if [ $RESULT -eq 0 ]; then
        print_status "========================================="
        print_status "TEST COMPILATION SUCCESSFUL"
        print_status "========================================="
        exit 0
    else
        print_error "========================================="
        print_error "TEST COMPILATION FAILED"
        print_error "========================================="
        exit 1
    fi
fi

# Full mode: compile all .c files
print_status "Compiling all source files..."
echo ""

SOURCES=(
    "circle-droplet.c"
    "droplet-impact-orifice.c"
    "droplet-impact-orifice-nondim.c"
    "droplet-impact-sharp-orifice.c"
    "droplet-impact-sharp-orifice-nondim.c"
    "droplet-impact-round-orifice.c"
)

FAILED=()
SUCCEEDED=()

for source in "${SOURCES[@]}"; do
    if [ -f "$source" ]; then
        if compile_file "$source"; then
            SUCCEEDED+=("$source")
        else
            FAILED+=("$source")
        fi
        echo ""
    else
        print_warning "Source file not found: $source (skipping)"
        echo ""
    fi
done

# Print summary
echo ""
echo "========================================="
echo "COMPILATION SUMMARY"
echo "========================================="
echo "Succeeded: ${#SUCCEEDED[@]}"
for file in "${SUCCEEDED[@]}"; do
    echo -e "  ${GREEN}✓${NC} $file"
done

if [ ${#FAILED[@]} -gt 0 ]; then
    echo ""
    echo "Failed: ${#FAILED[@]}"
    for file in "${FAILED[@]}"; do
        echo -e "  ${RED}✗${NC} $file"
    done
    echo ""
    print_error "========================================="
    print_error "COMPILATION FAILED FOR SOME FILES"
    print_error "========================================="
    exit 1
else
    echo ""
    print_status "========================================="
    print_status "ALL COMPILATIONS SUCCESSFUL"
    print_status "========================================="
    exit 0
fi
