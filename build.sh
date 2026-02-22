#!/usr/bin/env bash
# build.sh — Build the wandb_client project and run all tests.
#
# Usage:
#   ./build.sh            # Configure (if needed), build, and run tests
#   ./build.sh --clean    # Wipe the build directory first, then build and run
#
# Exit codes:
#   0  — all tests passed
#   1  — configure / build / test failure

set -euo pipefail

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
JOBS="$(sysctl -n hw.logicalcpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# Print a highlighted section header to stdout.
print_header() {
    echo ""
    echo "======================================================================"
    echo "  $*"
    echo "======================================================================"
}

# Print an error message and exit with code 1.
die() {
    echo "[ERROR] $*" >&2
    exit 1
}

# ---------------------------------------------------------------------------
# Option parsing
# ---------------------------------------------------------------------------
CLEAN=false
for arg in "$@"; do
    case "$arg" in
        --clean|-c)
            CLEAN=true
            ;;
        --help|-h)
            grep '^#' "$0" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            die "Unknown option: $arg"
            ;;
    esac
done

# ---------------------------------------------------------------------------
# Clean (optional)
# ---------------------------------------------------------------------------
if [[ "$CLEAN" == true ]]; then
    print_header "Cleaning build directory"
    rm -rf "${BUILD_DIR}"
    echo "Removed: ${BUILD_DIR}"
fi

# ---------------------------------------------------------------------------
# Configure
# ---------------------------------------------------------------------------
print_header "Configuring (CMake)"
cmake \
    -S "${SCRIPT_DIR}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    || die "CMake configuration failed."

# Keep the root-level compile_commands.json symlink up to date.
if [[ ! -e "${SCRIPT_DIR}/compile_commands.json" || \
      "$(readlink "${SCRIPT_DIR}/compile_commands.json")" != "${BUILD_DIR}/compile_commands.json" ]]; then
    ln -sf "${BUILD_DIR}/compile_commands.json" "${SCRIPT_DIR}/compile_commands.json"
fi

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
print_header "Building (${JOBS} parallel jobs)"
cmake --build "${BUILD_DIR}" --parallel "${JOBS}" \
    || die "Build failed."

# ---------------------------------------------------------------------------
# Test
# ---------------------------------------------------------------------------
print_header "Running tests (CTest)"
ctest \
    --test-dir "${BUILD_DIR}" \
    --output-on-failure \
    --parallel "${JOBS}" \
    || die "One or more tests failed."

print_header "SUCCESS — all tests passed"
