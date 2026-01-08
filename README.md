
# 🚀 SpaceMate: Smart Disk Manager

<div align="center">

**An intelligent, offline disk space manager built with OS-level system calls**

[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-blue.svg)](https://www.linux.org/)
[![Language: C/C++](https://img.shields.io/badge/Language-C%2FC%2B%2B-green.svg)](https://isocpp.org/)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)](http://makeapullrequest.com)

[Features](#-features) • [Demo](#-demo) • [Installation](#-installation) • [Usage](#-usage) • [Documentation](#-documentation) 

</div>

---

## 📖 About The Project

**SpaceMate** is a lightweight, privacy-first disk space management tool that operates at the OS level using system calls. Unlike bloated commercial alternatives, SpaceMate provides intelligent storage analysis, duplicate detection, and safe cleanup—all while running completely offline with zero third-party dependencies.

### 🎯 Problem Statement

Modern systems accumulate clutter over time:
- 📦 Duplicate files waste valuable storage
- 🗑️ Temporary files pile up unnoticed
- 📊 Users lack visibility into disk usage patterns
- ⚠️ Manual cleanup is time-consuming and risky

### 💡 Our Solution

SpaceMate automates intelligent disk management while keeping you in control:
- Real-time monitoring with visual insights
- Smart detection of redundant and unnecessary files
- Safe cleanup with automatic backups
- Zero installation, no login required, completely private

---

## ✨ Features

### 🔍 Intelligent Analysis
- **Real-time Disk Monitoring** - Track storage usage across all mounted filesystems
- **Directory Tree Visualization** - See what's consuming your space at a glance
- **Smart File Categorization** - Automatically group files by type, size, and age
- **Duplicate Detection** - Hash-based identification of duplicate files
- **Large File Scanner** - Quickly locate space hogs

### 🧹 Safe Cleanup
- **Interactive Cleanup Mode** - Review before you delete
- **Dry-Run Simulation** - Preview cleanup actions without making changes
- **Automatic Backups** - All deletions backed up before removal
- **Protected Directories** - System-critical paths are whitelisted
- **Operation Logging** - Complete audit trail of all actions

### 🔒 Privacy & Security
- **100% Offline** - No cloud, no tracking, no telemetry
- **Local Processing** - All analysis happens on your machine
- **Open Source** - Transparent, auditable code
- **No Root Required** - Runs with standard user permissions (for user directories)

### 🎨 User Experience
- **Dual Interface** - Both CLI and GUI options
- **Command-Line Interface** - Fast, scriptable, and lightweight
- **Graphical Interface** - Intuitive Qt5-based GUI with tabs
- **Color-Coded Output** - Easy-to-read terminal displays and visual storage breakdown
- **Progress Indicators** - Real-time progress for scanning and operations
- **Detailed Reports** - Activity logging and monitoring statistics
- **Cross-Platform** - Works on Linux and WSL2

---

## 🎬 Demo

### Quick Start Example

```bash
# Scan your Downloads folder
$ ./spacemate scan ~/Downloads

SpaceMate v1.0 - Smart Disk Manager
====================================
✓ Scanned 1,847 files in 2.4 seconds

Disk Usage: 39.2 GB / 50.0 GB (78.4%) [████████████████████░░░░░]
Free Space: 10.8 GB
⚠️  Warning: Disk usage above 75%
```

### Finding Duplicates

```bash
# Analyze for duplicates and waste
$ ./spacemate analyze ~/Downloads

📊 Analysis Complete!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Found Issues:
  🔄 47 duplicate files           → 6.2 GB wasted
  🗑️  23 temporary files          → 1.4 GB reclaimable
  ⏰ 89 old files (90+ days)     → 8.9 GB unused

💡 Potential Space Savings: 16.5 GB
```

### Safe Cleanup

```bash
# Preview cleanup (no changes made)
$ ./spacemate clean ~/Downloads --dry-run

Would delete:
  ✓ 44 duplicate copies          6.2 GB
  ✓ 23 temporary files           1.4 GB
  ✓ 15 cache files               892 MB

After cleanup:
  Free space: 10.8 GB → 19.3 GB
  Usage: 78.4% → 61.4%

# Proceed with actual cleanup
$ ./spacemate clean ~/Downloads

🔒 Creating backup...
✓ Backup saved to ~/.spacemate_backup/2025-10-03/
🗑️  Cleaning up... [████████████████████] 100%
✓ Removed 82 files, freed 8.5 GB
```

### Screenshots

<details>
<summary>📸 View Interface Screenshots (Click to expand)</summary>

#### CLI - Disk Scan
```
╔════════════════════════════════════════╗
║     SpaceMate - Smart Disk Manager     ║
║            Team Raptors #057           ║
╚════════════════════════════════════════╝

Usage: ./spacemate <command> <path> [options]

Commands:
  scan <path>       - Scan disk usage and show statistics
  analyze <path>    - Analyze files (duplicates, temp files, old files)
  clean <path>      - Clean up unnecessary files
  restore           - Restore backed up files
  help              - Show this help message
```

#### GUI - Dashboard Tab
```
╔════════════════════════════════════════╗
║           DISK USAGE                   ║
╠════════════════════════════════════════╣
║ Total: 256 GB                          ║
║ Used:  192 GB (75%)                    ║
║ Free:   64 GB                          ║
║                                        ║
║ [████████████████████░░░░░░░]          ║
║  Documents | Images | Apps | Free      ║
╚════════════════════════════════════════╝

[Start Monitoring] [Stop Monitoring]

Activity Log:
15:30:45 - Monitoring started for: /home/user/Downloads
15:30:46 - Found 1,234 files, 567 folders
15:30:46 - Documents: 234 files (2.4 GB)
15:30:46 - Images: 456 files (8.9 GB)
```

#### GUI - Analyze Tab
```
Path: /home/user/Downloads [Browse]
[Analyze]

Progress: [████████████████████] 100%
Status: Analysis complete

Results:
- Total files scanned: 1,234
- Duplicates found: 47 files (6.2 GB)
- Temp files: 23 files (1.4 GB)
- Old files (45+ days): 89 files (8.9 GB)
```

#### GUI - Cleanup Tab
```
Duplicate Files:
┌─────────────────────────────────────────┬──────────┐
│ File Path                                │ Size     │
├─────────────────────────────────────────┼──────────┤
│ /home/user/Downloads/movie_copy.mp4     │ 1.4 GB   │
│ /home/user/Downloads/movie_copy2.mp4    │ 1.4 GB   │
│ /home/user/Downloads/photo_backup.jpg   │ 5.2 MB   │
└─────────────────────────────────────────┴──────────┘

[Select All Duplicates] [Delete Selected Files]

Space to recover: 2.8 GB
```

#### GUI - Backup Tab
```
Existing Backups:
┌─────────────────────────┬─────────────┬──────────┐
│ Timestamp               │ File Path    │ Size     │
├─────────────────────────┼─────────────┼──────────┤
│ 2025-11-11 14:30:22    │ movie.mp4    │ 1.4 GB   │
│ 2025-11-11 14:30:25    │ photo.jpg    │ 5.2 MB   │
└─────────────────────────┴─────────────┴──────────┘

[Restore Selected]
```

</details>

---

## 🛠️ Installation

### Prerequisites

- **Operating System**: Linux (Ubuntu 20.04+, Debian, Fedora, Arch) or WSL2 on Windows
- **Compiler**: GCC/G++ 9.0 or higher with C++17 support
- **Build Tools**: CMake 3.10+, make
- **GUI Requirements**: Qt5 (qt5-default, qt5-qmake, qtbase5-dev)
- **Optional**: OpenSSL for enhanced hashing

### Quick Install

```bash
# Clone the repository
git clone https://github.com/itsmnx/Space-Mate.git
cd Space-Mate

# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential cmake qt5-default qtbase5-dev libqt5widgets5

# For WSL users, install X11 server support
# Download and install VcXsrv: https://sourceforge.net/projects/vcxsrv/

# Create build directory and compile
mkdir -p build
cd build
cmake ..
make -j4

# Verify build
ls -lh spacemate_cli SpacemateGUI
```

### Build Output

After successful build, you'll have:
- **spacemate_cli** (~295 KB) - Command-line executable
- **SpacemateGUI** (~802 KB) - Graphical user interface executable

### Building from Source

```bash
# Clean build
cd build
make clean
cmake ..
make -j4

# Build with verbose output
make VERBOSE=1

# Run from build directory
./spacemate_cli help
./SpacemateGUI  # (with DISPLAY=:0 in WSL)
```

---

## 📘 Usage

SpaceMate provides both **CLI** (Command-Line Interface) and **GUI** (Graphical User Interface) options.

### CLI Usage

#### Basic Commands

```bash
# Navigate to build directory
cd build

# Display help
./spacemate_cli help

# Scan a directory for disk usage
./spacemate_cli scan <path>

# Analyze files (duplicates, temp files, old files)
./spacemate_cli analyze <path>

# Clean up (with confirmation)
./spacemate_cli clean <path>

# Restore deleted files
./spacemate_cli restore
```

#### Advanced Options

```bash
# Verbose output
./spacemate_cli scan /path --verbose
./spacemate_cli analyze /path --verbose

# Dry run (preview only, no changes)
./spacemate_cli clean /path --dry-run

# Force mode (skip confirmations)
./spacemate_cli clean /path --force

# Combine options
./spacemate_cli clean /path --dry-run --verbose
```

### GUI Usage

#### Launching the GUI

**From WSL (Linux Environment):**
```bash
cd build
export DISPLAY=:0
./SpacemateGUI
```

**From Windows PowerShell:**
```powershell
# Using batch file (from project root)
.\spacemate.bat gui

# Or using WSL command directly
wsl -e bash -ic "cd /mnt/c/Users/YOUR_USERNAME/path/to/Spacemate/build && export DISPLAY=:0 && ./SpacemateGUI"
```

#### GUI Features

**Dashboard Tab:**
- View real-time disk usage statistics
- Color-coded storage visualization:
  - 🔵 **Blue**: Documents (PDF, DOCX, TXT, etc.)
  - 🟢 **Green**: Images (JPG, PNG, GIF, etc.)
  - 🟣 **Purple**: Applications & Executables
  - 🟠 **Orange**: System Files
  - ⚪ **Gray**: Free Space
- Monitor specific folders with file/folder counts
- Start/Stop real-time monitoring

**Analyze Tab:**
- Browse and select folders to scan
- Real-time progress updates during analysis
- Automatic detection of:
  - Duplicate files (MD5 hash-based)
  - Temporary files (.tmp, .temp, .log, .cache)
  - Old files (45+ days without modification)

**Cleanup Tab:**
- View detected duplicate files in a table
- Select individual files or use "Select All Duplicates"
- Preview total space savings before deletion
- Safe deletion with automatic backup
- Clear display with file paths and sizes

**Backup Tab:**
- Browse all backed-up files
- View backup timestamps and file sizes
- Restore individual files with one click
- Automatic backup index management

**Activity Log:**
- Real-time activity monitoring
- Detailed statistics:
  - Total files and folders in monitored location
  - File counts by category (Documents, Images, Videos, etc.)
  - Total size breakdown
- 24-hour timestamp format
- Auto-scroll to latest entries

#### GUI Prerequisites

**For Windows with WSL:**
1. Install an X11 server (VcXsrv recommended):
   - Download from: https://sourceforge.net/projects/vcxsrv/
   - Launch XLaunch
   - Choose "Multiple windows"
   - Display number: 0
   - Enable "Disable access control"

2. Verify X11 server is running:
   ```bash
   # In WSL terminal
   echo $DISPLAY  # Should show :0
   ```

### Configuration File

Create `~/.spacematerc` for custom settings:

```ini
# SpaceMate Configuration

[general]
backup_dir = ~/.spacemate/backups
log_level = info

[cleanup]
auto_backup = true
confirm_deletions = true
backup_retention_days = 30

[analysis]
duplicate_threshold = 100
old_file_days = 45
```

### Usage Examples

#### CLI Example: Clean Downloads Folder

```bash
cd build

# Step 1: See what's taking space
./spacemate_cli scan ~/Downloads

# Step 2: Find problems
./spacemate_cli analyze ~/Downloads

# Step 3: Preview cleanup
./spacemate_cli clean ~/Downloads --dry-run

# Step 4: Clean it up
./spacemate_cli clean ~/Downloads
```

#### GUI Example: Full Workflow

1. **Launch GUI**: Run `.\spacemate.bat gui` from project root
2. **Dashboard**: View current disk usage and storage breakdown
3. **Analyze**: 
   - Switch to Analyze tab
   - Click "Browse" to select folder
   - Click "Analyze" and wait for scan completion
4. **Cleanup**:
   - Switch to Cleanup tab
   - Review duplicate files
   - Click "Select All Duplicates" or select individual files
   - Click "Delete Selected Files"
   - Confirm deletion
5. **Monitor**: 
   - Return to Dashboard
   - Click "Start Monitoring" to track folder statistics in real-time
   - View updates in Activity Log
6. **Restore** (if needed):
   - Go to Backup tab
   - Select file to restore
   - Click "Restore Selected"

---

## 🏗️ Architecture

### High-Level Design

```
┌─────────────────────────────────────────────────────────────┐
│                   User Interface Layer                       │
│         ┌──────────────────┐    ┌──────────────────┐        │
│         │   CLI Interface  │    │   GUI (Qt5)      │        │
│         │   (main.cpp)     │    │   (mainwindow)   │        │
│         └────────┬─────────┘    └────────┬─────────┘        │
└──────────────────┼──────────────────────┼──────────────────┘
                   │                       │
         ┌─────────┴───────────────────────┴─────────┐
         │                                            │
┌────────▼──────────┐    ┌───────▼────────┐   ┌─────▼──────────┐
│   Disk Monitor    │    │ File Analyzer  │   │ Cleanup Manager│
│   - QStorageInfo  │    │ - QDir/QFile   │   │ - Safe delete  │
│   - statvfs()     │    │ - MD5 hashing  │   │ - File backup  │
│   - Filesystem    │    │ - Categorize   │   │ - Logging      │
│   statistics      │    │ - Duplicates   │   │                │
└────────┬──────────┘    └────────┬────────┘   └────────┬───────┘
         │                        │                      │
         └────────────────────────┼──────────────────────┘
                                  │
                         ┌────────▼─────────┐
                         │ Backup Manager   │
                         │ - Create backups │
                         │ - Restore files  │
                         │ - Index tracking │
                         └──────────────────┘
```

### Module Breakdown

| Module | Files | Responsibility | Key Technologies |
|--------|------|----------------|------------------|
| **CLI Interface** | `main.cpp` | Command-line argument parsing | C++ iostream, getopt |
| **GUI Interface** | `gui/mainwindow.cpp`, `gui/mainwindow.h` | Graphical user interface | Qt5 Widgets, Signals/Slots |
| **Disk Monitor** | `core/disk_monitor.cpp`, `include/disk_monitor.h` | Track filesystem statistics | QStorageInfo, statvfs() |
| **File Analyzer** | `core/file_analyzer.cpp`, `include/file_analyzer.h` | Scan, categorize, detect duplicates | QCryptographicHash, std::filesystem |
| **Cleanup Manager** | `core/cleanup_manager.cpp`, `include/cleanup_manager.h` | Safe file deletion with backups | QFile::remove(), logging |
| **Backup Manager** | `core/backup_manager.cpp`, `include/backup_manager.h` | Backup & restore operations | QFile::copy(), JSON index |
| **Utilities** | `core/utils.cpp`, `include/utils.h` | Helper functions | String formatting, file ops |

### Key Algorithms

#### 1. Duplicate Detection
```
Algorithm: MD5 Hash-Based Duplicate Detection
Time Complexity: O(n log n)
Space Complexity: O(n)

1. Scan all files in directory (recursive)
2. For each file:
   - Compute MD5 hash using QCryptographicHash
   - Store in hash map: hash -> list of file paths
3. Identify hash collisions (same hash = duplicates)
4. Group duplicates and report to user
```

#### 2. Directory Scanning
```
Algorithm: Recursive Filesystem Traversal
Time Complexity: O(n) where n = number of files
Space Complexity: O(d) where d = max directory depth

1. Start from root directory using QDir
2. For each entry:
   - If file: collect metadata (size, mtime, path)
   - If directory: recurse into subdirectory
3. Categorize files by extension
4. Accumulate sizes and statistics
```

#### 3. Safe Cleanup with Backup
```
Algorithm: Two-Phase Cleanup
Phase 1: Backup
  - For each file to delete:
    * Create backup directory (~/.spacemate/backups/)
    * Copy file to backup location
    * Record metadata in backup index
    * Verify backup success

Phase 2: Delete
  - Verify all backups successful
  - Delete original files using QFile::remove()
  - Log all operations with timestamp
  - Update activity log
  - On failure: halt and report error
```

---

## 🧪 Testing

### Running Tests

```bash
# Navigate to tests directory
cd tests

# Run test script
./run_tests.sh

# Manual testing of CLI
cd ../build
./spacemate_cli help
./spacemate_cli scan /tmp
./spacemate_cli analyze /tmp

# Manual testing of GUI
export DISPLAY=:0
./SpacemateGUI
```

### Test Scenarios

**CLI Testing:**
```bash
# Test 1: Basic help
./spacemate_cli help

# Test 2: Scan directory
./spacemate_cli scan ~/Downloads

# Test 3: Analyze for duplicates
./spacemate_cli analyze ~/Documents --verbose

# Test 4: Dry-run cleanup
./spacemate_cli clean ~/temp --dry-run

# Test 5: Actual cleanup
./spacemate_cli clean ~/temp

# Test 6: Restore backup
./spacemate_cli restore
```

**GUI Testing:**
1. Launch GUI successfully
2. Navigate all tabs (Dashboard, Analyze, Cleanup, Backup)
3. Scan a folder with "Analyze"
4. View duplicates in Cleanup tab
5. Select and delete files
6. Verify backup in Backup tab
7. Restore a file
8. Start/stop monitoring
9. Check activity log updates

---

## 🤝 Contributing

We welcome contributions from the community! Here's how you can help:

### How to Contribute

1. **Fork the repository**
2. **Create a feature branch** (`git checkout -b feature/AmazingFeature`)
3. **Commit your changes** (`git commit -m 'Add some AmazingFeature'`)
4. **Push to the branch** (`git push origin feature/AmazingFeature`)
5. **Open a Pull Request**

### Development Setup

```bash
# Clone your fork
git clone https://github.com/your-username/spacemate.git
cd spacemate

# Create development branch
git checkout -b dev

# Make changes and test
make clean && make debug
make test

# Commit with meaningful messages
git commit -m "feat: add new duplicate detection algorithm"
```

### Coding Standards

- Follow K&R C style guidelines
- Add comments for complex logic
- Write unit tests for new features
- Update documentation
- Keep functions under 50 lines when possible

### Areas for Contribution

- 🐛 Bug fixes and issue resolution
- ✨ New features (see [Issues](https://github.com/team-raptors/spacemate/issues))
- 📝 Documentation improvements
- 🧪 Additional test coverage
- 🌐 Localization and i18n
- 🎨 GUI development (Qt/GTK)

---

## 📊 Performance

### Build Information

- **CLI Executable**: ~295 KB
- **GUI Executable**: ~802 KB
- **Build Time**: ~15-20 seconds (with make -j4)
- **Qt Version**: Qt 5.x
- **C++ Standard**: C++17

### Runtime Performance

| Operation | Scale | Expected Performance |
|-----------|-------|---------------------|
| Scan Directory | 1,000 files | < 2 seconds |
| Scan Directory | 10,000 files | < 10 seconds |
| Duplicate Detection | 1,000 files | < 5 seconds |
| MD5 Hash Calculation | Per file | ~1-2 MB/ms |
| File Cleanup | 100 files | < 1 second |
| GUI Launch | - | < 2 seconds |

### Optimization Features

- Multi-threaded scanning for large directories
- Efficient MD5 hashing with QCryptographicHash
- Lazy loading for GUI tables
- Real-time progress updates during operations
- Minimal memory footprint (~50-100 MB typical usage)

---

## 🔧 Troubleshooting

### Common Issues

**Issue**: Permission denied errors
```bash
# Solution: Run with appropriate permissions
./spacemate_cli scan /home/user  # Works
./spacemate_cli scan /root       # Needs sudo
```

**Issue**: GUI doesn't launch in WSL
```bash
# Solution 1: Check X11 server is running
# - Launch VcXsrv on Windows
# - Verify display: echo $DISPLAY (should show :0)

# Solution 2: Set DISPLAY environment variable
export DISPLAY=:0
./SpacemateGUI

# Solution 3: Use batch file from project root
# (In Windows PowerShell)
.\spacemate.bat gui
```

**Issue**: Qt permission warning in WSL
```
QStandardPaths: wrong permissions on runtime directory /run/user/1000/
```
```bash
# Solution: This is harmless and can be ignored
# It does not affect functionality
# Or fix permissions:
chmod 0700 /run/user/1000/
```

**Issue**: Build errors with Qt
```bash
# Solution: Install Qt5 development packages
sudo apt install qt5-default qtbase5-dev libqt5widgets5

# Rebuild
cd build
cmake ..
make clean && make -j4
```

**Issue**: "spacemate_cli: command not found"
```bash
# Solution: Make sure you're in the build directory
cd /path/to/Spacemate/build
./spacemate_cli help  # Note the ./
```

### Debug Mode

```bash
# Check build output
cd build
ls -lh spacemate_cli SpacemateGUI

# Test CLI
./spacemate_cli help

# Test GUI (with verbose X11 output)
export DISPLAY=:0
./SpacemateGUI --verbose

# Check activity logs in GUI
# Navigate to Dashboard > Activity Log tab
```

### Getting Help

- 📧 Email: jmanas275@gmail.com
- 🐛 Issues: [GitHub Issues](https://github.com/itsmnx/Space-Mate/issues)
- � Repository: [GitHub](https://github.com/itsmnx/Space-Mate)
- � Documentation: See [run.md](run.md) for detailed running instructions

---

## 📚 Documentation

- **[Running Instructions](run.md)** - Detailed CLI and GUI commands
- **[Setup Guide](setup.sh)** - Automated setup script for dependencies
- **[CMake Configuration](CMakeLists.txt)** - Build system configuration
- **Project Structure**:
  - `build/` - Compiled executables (spacemate_cli, SpacemateGUI)
  - `core/` - Core implementation files (.cpp)
  - `gui/` - GUI implementation (Qt5)
  - `include/` - Header files (.h)
  - `logs/` - Application logs
  - `tests/` - Test scripts

---

## 🗺️ Roadmap

### Phase 1: Core Functionality (✅ Complete)
- [x] CLI disk usage monitoring
- [x] Directory scanning and analysis
- [x] MD5-based duplicate detection
- [x] Safe cleanup with backup
- [x] Backup and restore system
- [x] Qt5 GUI interface
- [x] Real-time monitoring
- [x] Activity logging
- [x] Color-coded storage visualization

### Phase 2: Enhanced Features (🚧 In Progress)
- [x] Segmented storage breakdown by file type
- [x] Auto-delete files older than 45 days
- [x] Folder-specific monitoring
- [ ] Scheduled cleanup tasks
- [ ] Advanced filtering options
- [ ] Configuration profiles
- [ ] Export reports to CSV/JSON

### Phase 3: Advanced Capabilities (📋 Planned)
- [ ] Real filesystem scanning for accurate categorization
- [ ] Machine learning for smart recommendations
- [ ] Network drive support
- [ ] Multi-user support
- [ ] Plugin system
- [ ] Windows native GUI (non-WSL)

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2025 Team Raptors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

---

## 🙏 Acknowledgments

- **Course**: Operating Systems (OS-T057)
- **Institution**: BTech Computer Science & Engineering
- **Inspired by**: Unix utilities (du, df, fsck)
- **References**:
  - A. S. Tanenbaum - *Modern Operating Systems*
  - W. Stallings - *Operating Systems: Internals and Design Principles*
  - Linux Kernel Documentation

### Special Thanks

- Our course instructors for guidance
- The open-source community for inspiration
- Linux kernel developers for excellent documentation

---

## 📞 Contact & Support

### Get in Touch

- **Email**: jmanas275@gmail.com
  
### Stay Updated

- ⭐ Star this repo to show support
- 👀 Watch for updates and releases
- 🍴 Fork to contribute

---

---

<div align="center">

**Made with ❤️ by Team Raptors**

*If you found SpaceMate helpful, please consider giving it a ⭐!*

[⬆ Back to Top](#-spacemate-smart-disk-manager)

</div>
