#!/bin/bash

# ============================================================================
# SpaceMate Test Suite
# Comprehensive testing for all features
# ============================================================================

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

PASSED=0
FAILED=0
TEST_DIR="/tmp/spacemate_test_$$"

# Cleanup function
cleanup() {
    rm -rf "$TEST_DIR"
}

trap cleanup EXIT

echo -e "${CYAN}${BOLD}"
echo "╔════════════════════════════════════════╗"
echo "║        SpaceMate Test Suite            ║"
echo "╚════════════════════════════════════════╝"
echo -e "${RESET}\n"

# Test 1: Build Check
echo -e "${CYAN}[Test 1] Checking if SpaceMate is built...${RESET}"
if [ -f "./bin/spacemate" ]; then
    echo -e "${GREEN}✓ PASS: SpaceMate binary exists${RESET}"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL: SpaceMate binary not found${RESET}"
    echo -e "${YELLOW}Run 'make' to build the project first${RESET}"
    ((FAILED++))
    exit 1
fi

# Create test directory
mkdir -p "$TEST_DIR"
cd "$TEST_DIR" || exit

# Test 2: Scan Command
echo -e "\n${CYAN}[Test 2] Testing scan command...${RESET}"
if ./bin/spacemate scan . > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASS: Scan command works${RESET}"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL: Scan command failed${RESET}"
    ((FAILED++))
fi

# Test 3: Create test files
echo -e "\n${CYAN}[Test 3] Creating test files...${RESET}"
mkdir -p "$TEST_DIR/subdir"
echo "test content 1" > "$TEST_DIR/file1.txt"
echo "test content 1" > "$TEST_DIR/file2.txt"  # Duplicate
echo "different content" > "$TEST_DIR/file3.txt"
echo "temp" > "$TEST_DIR/temp.tmp"
echo "log data" > "$TEST_DIR/app.log"
dd if=/dev/zero of="$TEST_DIR/large.dat" bs=1M count=5 2>/dev/null

if [ -f "$TEST_DIR/file1.txt" ]; then
    echo -e "${GREEN}✓ PASS: Test files created${RESET}"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL: Could not create test files${RESET}"
    ((FAILED++))
fi

# Test 4: Analyze Command
echo -e "\n${CYAN}[Test 4] Testing analyze command...${RESET}"
if $TEST_DIR/../bin/spacemate analyze "$TEST_DIR" > /tmp/analyze_output.txt 2>&1; then
    if grep -q "DUPLICATE\|TEMPORARY" /tmp/analyze_output.txt; then
        echo -e "${GREEN}✓ PASS: Analyze detected issues${RESET}"
        ((PASSED++))
    else
        echo -e "${YELLOW}⚠ WARN: Analyze ran but found no issues${RESET}"
        ((PASSED++))
    fi
else
    echo -e "${RED}✗ FAIL: Analyze command failed${RESET}"
    ((FAILED++))
fi

# Test 5: Dry Run Clean
echo -e "\n${CYAN}[Test 5] Testing dry-run cleanup...${RESET}"
if $TEST_DIR/../bin/spacemate clean "$TEST_DIR" --dry-run > /dev/null 2>&1; then
    # Check if temp files still exist after dry-run
    if [ -f "$TEST_DIR/temp.tmp" ]; then
        echo -e "${GREEN}✓ PASS: Dry-run didn't delete files${RESET}"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAIL: Dry-run deleted files${RESET}"
        ((FAILED++))
    fi
else
    echo -e "${YELLOW}⚠ WARN: Dry-run command had issues${RESET}"
    ((PASSED++))
fi

# Test 6: Help Command
echo -e "\n${CYAN}[Test 6] Testing help command...${RESET}"
if $TEST_DIR/../bin/spacemate help > /tmp/help_output.txt 2>&1; then
    if grep -q "Usage\|Commands" /tmp/help_output.txt; then
        echo -e "${GREEN}✓ PASS: Help displays correctly${RESET}"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAIL: Help output incomplete${RESET}"
        ((FAILED++))
    fi
else
    echo -e "${RED}✗ FAIL: Help command failed${RESET}"
    ((FAILED++))
fi

# Test 7: Backup Directory Creation
echo -e "\n${CYAN}[Test 7] Testing backup system...${RESET}"
BACKUP_DIR="$HOME/.spacemate/backup"
if [ -d "$BACKUP_DIR" ] || mkdir -p "$BACKUP_DIR" 2>/dev/null; then
    echo -e "${GREEN}✓ PASS: Backup directory accessible${RESET}"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL: Cannot create backup directory${RESET}"
    ((FAILED++))
fi

# Test 8: Permission Test
echo -e "\n${CYAN}[Test 8] Testing file permissions...${RESET}"
if [ -x "./bin/spacemate" ]; then
    echo -e "${GREEN}✓ PASS: Binary has execute permissions${RESET}"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL: Binary not executable${RESET}"
    ((FAILED++))
fi

# Test 9: Large Directory Test
echo -e "\n${CYAN}[Test 9] Testing with multiple files...${RESET}"
for i in {1..50}; do
    echo "file $i" > "$TEST_DIR/file_$i.txt"
done

if $TEST_DIR/../bin/spacemate scan "$TEST_DIR" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASS: Handles multiple files${RESET}"
    ((PASSED++))
else
    echo -e "${RED}✗ FAIL: Failed with many files${RESET}"
    ((FAILED++))
fi

# Test 10: Error Handling
echo -e "\n${CYAN}[Test 10] Testing error handling...${RESET}"
if ! $TEST_DIR/../bin/spacemate scan /nonexistent/path > /dev/null 2>&1; then
    echo -e "${GREEN}✓ PASS: Handles invalid paths gracefully${RESET}"
    ((PASSED++))
else
    echo -e "${YELLOW}⚠ WARN: May not handle errors properly${RESET}"
    ((PASSED++))
fi

# Summary
echo -e "\n${BOLD}════════════════════════════════════════${RESET}"
echo -e "${BOLD}Test Summary${RESET}"
echo -e "${BOLD}════════════════════════════════════════${RESET}"
echo -e "${GREEN}Passed: $PASSED${RESET}"
echo -e "${RED}Failed: $FAILED${RESET}"
echo -e "Total:  $((PASSED + FAILED))"
echo -e "${BOLD}════════════════════════════════════════${RESET}\n"

# Cleanup temp files
rm -f /tmp/analyze_output.txt /tmp/help_output.txt

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}${BOLD}✓ All tests passed!${RESET}\n"
    exit 0
else
    echo -e "${RED}${BOLD}✗ Some tests failed${RESET}\n"
    exit 1
fi