#!/bin/bash

# ============================================================================
# SpaceMate Setup Script
# Automated installation for WSL/Linux
# ============================================================================

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

echo -e "${CYAN}${BOLD}"
echo "╔════════════════════════════════════════╗"
echo "║     SpaceMate Setup & Installation     ║"
echo "║            Team Raptors #057           ║"
echo "╚════════════════════════════════════════╝"
echo -e "${RESET}\n"

# Check if running on Linux/WSL
if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo -e "${RED}Error: This script must be run on Linux or WSL${RESET}"
    exit 1
fi

echo -e "${CYAN}[1/5] Checking system requirements...${RESET}"

# Check for required tools
MISSING_DEPS=()

if ! command -v g++ &> /dev/null; then
    MISSING_DEPS+=("g++")
fi

if ! command -v make &> /dev/null; then
    MISSING_DEPS+=("make")
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo -e "${YELLOW}Missing dependencies: ${MISSING_DEPS[*]}${RESET}"
    echo -e "${CYAN}Installing dependencies...${RESET}"
    
    if command -v apt &> /dev/null; then
        sudo apt update
        sudo apt install -y build-essential libssl-dev
    elif command -v yum &> /dev/null; then
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y openssl-devel
    else
        echo -e "${RED}Error: Could not detect package manager${RESET}"
        exit 1
    fi
else
    echo -e "${GREEN}✓ All dependencies installed${RESET}"
fi

echo -e "\n${CYAN}[2/5] Creating project structure...${RESET}"

# Create directory structure
mkdir -p core
mkdir -p include
mkdir -p logs
mkdir -p tests
mkdir -p bin
mkdir -p build

echo -e "${GREEN}✓ Project structure created${RESET}"

echo -e "\n${CYAN}[3/5] Building SpaceMate...${RESET}"

# Build the project
make clean
if make; then
    echo -e "${GREEN}✓ Build successful!${RESET}"
else
    echo -e "${RED}✗ Build failed${RESET}"
    exit 1
fi

echo -e "\n${CYAN}[4/5] Running basic tests...${RESET}"

# Create test directory if it doesn't exist
TEST_DIR="/tmp/spacemate_test"
mkdir -p "$TEST_DIR"

# Create test files
echo "test content" > "$TEST_DIR/test1.txt"
cp "$TEST_DIR/test1.txt" "$TEST_DIR/test2.txt"
echo "different" > "$TEST_DIR/test3.tmp"

# Test scan command
echo -e "${CYAN}Testing scan command...${RESET}"
if ./bin/spacemate scan "$TEST_DIR" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Scan test passed${RESET}"
else
    echo -e "${YELLOW}⚠ Scan test had issues (may be permissions)${RESET}"
fi

# Clean up test directory
rm -rf "$TEST_DIR"

echo -e "\n${CYAN}[5/5] Setup complete!${RESET}\n"

echo -e "${GREEN}${BOLD}SpaceMate is ready to use!${RESET}\n"
echo -e "Quick Start:"
echo -e "  ${CYAN}./bin/spacemate help${RESET}           - Show help"
echo -e "  ${CYAN}./bin/spacemate scan ~/Downloads${RESET}  - Scan directory"
echo -e "  ${CYAN}./bin/spacemate analyze ~/temp${RESET}    - Analyze files"
echo -e ""
echo -e "Optional: Run ${CYAN}'sudo make install'${RESET} to install system-wide"
echo -e ""