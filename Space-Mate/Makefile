# ============================================================================
# SpaceMate Makefile
# ============================================================================

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I./include
LDFLAGS = -lssl -lcrypto

# Directories
SRC_DIR = core
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(BIN_DIR)/spacemate

# Main source
MAIN_SRC = main.cpp
MAIN_OBJ = $(BUILD_DIR)/main.o

# Colors for output
GREEN = \033[0;32m
CYAN = \033[0;36m
RESET = \033[0m

.PHONY: all clean install uninstall test help setup

all: setup $(TARGET)

setup:
	@echo "$(CYAN)Setting up SpaceMate build environment...$(RESET)"
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p logs
	@mkdir -p tests

$(TARGET): $(OBJECTS) $(MAIN_OBJ)
	@echo "$(GREEN)Linking $@...$(RESET)"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)✓ Build complete! Run './bin/spacemate help' to get started$(RESET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "$(CYAN)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAIN_OBJ): $(MAIN_SRC)
	@echo "$(CYAN)Compiling main.cpp...$(RESET)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "$(CYAN)Cleaning build files...$(RESET)"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
	@echo "$(GREEN)✓ Clean complete$(RESET)"

install: all
	@echo "$(CYAN)Installing SpaceMate...$(RESET)"
	@sudo cp $(TARGET) /usr/local/bin/spacemate
	@sudo chmod +x /usr/local/bin/spacemate
	@echo "$(GREEN)✓ Installed to /usr/local/bin/spacemate$(RESET)"
	@echo "You can now run 'spacemate' from anywhere!"

uninstall:
	@echo "$(CYAN)Uninstalling SpaceMate...$(RESET)"
	@sudo rm -f /usr/local/bin/spacemate
	@echo "$(GREEN)✓ Uninstalled$(RESET)"

test: all
	@echo "$(CYAN)Running tests...$(RESET)"
	@bash tests/run_tests.sh

help:
	@echo "SpaceMate Build System"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build SpaceMate (default)"
	@echo "  clean     - Remove build files"
	@echo "  install   - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from system"
	@echo "  test      - Run test suite"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "After building, run: ./bin/spacemate help"