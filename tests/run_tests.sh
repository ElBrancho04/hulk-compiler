#!/bin/bash
# Run all smoke tests for the Hulk compiler
# Compiles tests if needed, runs each, and reports results

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

BUILD_DIR="./build"
PASSED=0
FAILED=0
TOTAL=0

# List of test targets (must match CMakeLists.txt targets)
TEST_TARGETS=(
    "bytecode_smoke_test"
    "codegen_smoke_test"
    "vm_smoke_test"
    "for_loop_e2e_test"
    "type_table_smoke_test"
    "semantic_utils_smoke_test"
    "semantic_analyzer_smoke_test"
    "lambda_smoke_test"
    "type_annotations_smoke_test"
    "runtime_test"
    "functor_smoke_test"
    "macro_expander_test"
)

# Step 1: Build tests if needed
echo -e "${YELLOW}=== Building tests ===${NC}"

if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    echo "Creating build directory and running CMake..."
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake .. 2>&1 | tail -5
    cd ..
fi

echo "Building all test targets..."
if ! cmake --build "${BUILD_DIR}" 2>&1 | tail -5; then
    echo -e "${RED}ERROR: Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful.${NC}"
echo ""

# Step 2: Run each test
echo -e "${YELLOW}=== Running tests ===${NC}"
echo ""

for target in "${TEST_TARGETS[@]}"; do
    TOTAL=$((TOTAL + 1))
    test_binary="${BUILD_DIR}/${target}"

    if [ ! -f "${test_binary}" ]; then
        echo -e "[${RED}SKIP${NC}] ${target} - binary not found"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Run the test and capture output and exit code
    output=$("${test_binary}" 2>&1) || true
    exit_code=$?

    if [ ${exit_code} -eq 0 ]; then
        echo -e "[${GREEN}PASS${NC}] ${target}"
        PASSED=$((PASSED + 1))
    else
        echo -e "[${RED}FAIL${NC}] ${target} (exit code: ${exit_code})"
        echo "  Output:"
        echo "${output}" | head -20 | sed 's/^/    /'
        FAILED=$((FAILED + 1))
    fi
done

# Step 3: Integration tests (.hulk files through full pipeline)
echo ""
echo -e "${YELLOW}=== Integration tests ===${NC}"
echo ""

HULK_BIN="${BUILD_DIR}/hulk"
HULKVM_BIN="${BUILD_DIR}/hulk-vm"
INTEGRATION_DIR="$(dirname "$0")"

if [ ! -f "${HULK_BIN}" ] || [ ! -f "${HULKVM_BIN}" ]; then
    echo -e "${YELLOW}SKIP${NC} integration tests: hulk or hulk-vm not found"
else
    run_integration() {
        local test_file="$1"
        local expected="$2"
        TOTAL=$((TOTAL + 1))
        local name
        name="$(basename "${test_file}")"
        local actual
        actual=$("${HULK_BIN}" "${test_file}" 2>&1 && "${HULKVM_BIN}" compiled.asm 2>&1) || true
        if echo "${actual}" | grep -qF "${expected}"; then
            echo -e "[${GREEN}PASS${NC}] integration: ${name}"
            PASSED=$((PASSED + 1))
        else
            echo -e "[${RED}FAIL${NC}] integration: ${name}"
            echo "  Expected to contain: ${expected}"
            echo "  Got: ${actual}" | head -5 | sed 's/^/    /'
            FAILED=$((FAILED + 1))
        fi
    }

    run_integration "${INTEGRATION_DIR}/test_macros.hulk" "macros OK"
    run_integration "${INTEGRATION_DIR}/test_match.hulk" "match OK"
    run_integration "${INTEGRATION_DIR}/test_full.hulk" ""
fi

echo ""
echo -e "${YELLOW}=== Test Summary ===${NC}"
echo -e "Total: ${TOTAL}"
echo -e "${GREEN}Passed: ${PASSED}${NC}"
echo -e "${RED}Failed: ${FAILED}${NC}"

if [ ${FAILED} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi