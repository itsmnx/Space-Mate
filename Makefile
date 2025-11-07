# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include
LDFLAGS = -lstdc++fs

# Qt Configuration
QT_PATH = /usr/lib/x86_64-linux-gnu/qt5
QT_INCLUDE = -I/usr/include/x86_64-linux-gnu/qt5 \
             -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets \
             -I/usr/include/x86_64-linux-gnu/qt5/QtGui \
             -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
             -I/usr/include/x86_64-linux-gnu/qt5/QtConcurrent

QT_LIBS = -lQt5Widgets -lQt5Gui -lQt5Core -lpthread
MOC = /usr/bin/moc

# Directories
CORE_DIR = core
GUI_DIR = gui
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
CORE_SOURCES = $(CORE_DIR)/backup_manager.cpp \
               $(CORE_DIR)/cleanup_manager.cpp \
               $(CORE_DIR)/disk_monitor.cpp \
               $(CORE_DIR)/file_analyzer.cpp \
               $(CORE_DIR)/utils.cpp

GUI_SOURCES = $(GUI_DIR)/mainwindow.cpp \
              $(GUI_DIR)/guimain.cpp

CLI_SOURCE = tests/main.cpp

# Object files
CORE_OBJECTS = $(patsubst $(CORE_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CORE_SOURCES))
GUI_OBJECTS = $(patsubst $(GUI_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(GUI_SOURCES))
CLI_OBJECT = $(BUILD_DIR)/main.o

# MOC files
GUI_MOC = $(BUILD_DIR)/moc_mainwindow.cpp
GUI_MOC_OBJECT = $(BUILD_DIR)/moc_mainwindow.o

# Executables
CLI_EXEC = $(BIN_DIR)/spacemate
GUI_EXEC = $(BIN_DIR)/spacemate-gui

# Targets
.PHONY: all cli gui clean directories

all: directories cli gui

cli: directories $(CLI_EXEC)

gui: directories $(GUI_EXEC)

directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# CLI executable
$(CLI_EXEC): $(CORE_OBJECTS) $(CLI_OBJECT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "CLI built successfully: $(CLI_EXEC)"

# GUI executable
$(GUI_EXEC): $(CORE_OBJECTS) $(GUI_OBJECTS) $(GUI_MOC_OBJECT)
	$(CXX) $(CXXFLAGS) $(QT_INCLUDE) -o $@ $^ $(LDFLAGS) $(QT_LIBS)
	@echo "GUI built successfully: $(GUI_EXEC)"

# Core object files
$(BUILD_DIR)/%.o: $(CORE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# CLI object file
$(BUILD_DIR)/main.o: $(CLI_SOURCE)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# GUI object files
$(BUILD_DIR)/%.o: $(GUI_DIR)/%.cpp $(GUI_MOC)
	$(CXX) $(CXXFLAGS) $(QT_INCLUDE) -fPIC -c -o $@ $<

# MOC file generation
$(GUI_MOC): $(GUI_DIR)/mainwindow.h
	$(MOC) -I/usr/include/x86_64-linux-gnu/qt5 \
	        -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets \
	        -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
	        -I/usr/include/x86_64-linux-gnu/qt5/QtGui \
	        $< -o $@


# MOC object file
$(GUI_MOC_OBJECT): $(GUI_MOC)
	$(CXX) $(CXXFLAGS) $(QT_INCLUDE) -fPIC -c -o $@ $<

# Clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Cleaned build files"

# Install (optional)
install: all
	@echo "Installing Spacemate..."
	sudo cp $(CLI_EXEC) /usr/local/bin/
	sudo cp $(GUI_EXEC) /usr/local/bin/
	@echo "Installation complete"

# Uninstall (optional)
uninstall:
	sudo rm -f /usr/local/bin/spacemate
	sudo rm -f /usr/local/bin/spacemate-gui
	@echo "Uninstalled"

# Help
help:
	@echo "Spacemate Build System"
	@echo "======================"
	@echo "Targets:"
	@echo "  all       - Build both CLI and GUI (default)"
	@echo "  cli       - Build CLI version only"
	@echo "  gui       - Build GUI version only"
	@echo "  clean     - Remove build files"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo ""
	@echo "Usage:"
	@echo "  make           # Build everything"
	@echo "  make cli       # Build CLI only"
	@echo "  make gui       # Build GUI only"
	@echo "  ./bin/spacemate     # Run CLI"
	@echo "  ./bin/spacemate-gui # Run GUI"