
# 🚀 SpaceMate: Smart Disk Manager

<div align="center">

**An intelligent, offline disk space manager built with OS-level system calls**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
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
- **Command-Line Interface** - Fast and scriptable
- **Color-Coded Output** - Easy-to-read terminal displays
- **Progress Indicators** - Never wonder if it's working
- **Detailed Reports** - Save analysis results to file

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
<summary>📸 View Screenshots (Click to expand)</summary>

#### Disk Scan
```
╔════════════════════════════════════════╗
║        DISK USAGE STATISTICS          ║
╚════════════════════════════════════════╝
Total Size:     50.0 GB
Used Space:     39.2 GB  [████████████████████░░░░░] 78.4%
Free Space:     10.8 GB
```

#### File Analysis
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 FILE DISTRIBUTION BY TYPE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Videos      15.2 GB  (38.8%)  ████████
Archives    12.4 GB  (31.6%)  ███████
Documents    5.8 GB  (14.8%)  ███
```

#### Duplicate Detection
```
🔄 DUPLICATE FILES DETECTED
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Group 1: movie.mp4 (3 copies)
  ├─ movie.mp4              1.4 GB [KEEP]
  ├─ movie_copy.mp4         1.4 GB [DELETE]
  └─ movie_copy2.mp4        1.4 GB [DELETE]
  Savings: 2.8 GB
```

</details>

---

## 🛠️ Installation

### Prerequisites

- **Operating System**: Linux (Ubuntu 20.04+, Debian, Fedora, Arch) or WSL2
- **Compiler**: GCC/G++ 9.0 or higher
- **Build Tools**: make, standard C libraries
- **Optional**: OpenSSL for enhanced hashing

### Quick Install

```bash
# Clone the repository
git clone https://github.com/team-raptors/spacemate.git
cd spacemate

# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential libssl-dev

# Build the project
make

# Run SpaceMate
./spacemate --help
```

### Building from Source

```bash
# Build with debug symbols
make debug

# Build optimized release version
make release

# Run tests
make test

# Install system-wide (optional)
sudo make install
```

### WSL Installation

```bash
# On Windows with WSL2
wsl --install -d Ubuntu
wsl

# Then follow Quick Install steps above
```

---

## 📘 Usage

### Command Reference

#### Basic Commands

```bash
# Display help
./spacemate --help

# Check version
./spacemate --version

# Scan a directory
./spacemate scan <path>

# Analyze files
./spacemate analyze <path>

# Clean up (with confirmation)
./spacemate clean <path>

# Restore deleted files
./spacemate restore
```

#### Advanced Options

```bash
# Scan with detailed output
./spacemate scan /home/user --verbose

# Analyze files older than 60 days
./spacemate analyze /downloads --older-than 60

# Clean files larger than 100MB only
./spacemate clean /temp --min-size 100M

# Dry run (preview only, no changes)
./spacemate clean /downloads --dry-run

# Non-interactive mode (for scripts)
./spacemate clean /temp --force --no-backup

# Export report to file
./spacemate analyze /home --output report.txt

# Exclude specific directories
./spacemate scan /home --exclude node_modules,.git

# Show only duplicates
./spacemate analyze /downloads --duplicates-only
```

### Configuration File

Create `~/.spacematerc` for custom settings:

```ini
# SpaceMate Configuration

[general]
backup_dir = ~/.spacemate_backup
log_level = info
color_output = true

[cleanup]
auto_backup = true
confirm_deletions = true
backup_retention_days = 30

[analysis]
min_file_size = 1M
duplicate_threshold = 100
temp_file_patterns = *.tmp,*.log,*.cache

[exclude]
directories = .git,node_modules,.venv,__pycache__
file_extensions = .sys,.dll
```

### Usage Examples

#### Example 1: Clean Downloads Folder

```bash
# Step 1: See what's taking space
./spacemate scan ~/Downloads

# Step 2: Find problems
./spacemate analyze ~/Downloads

# Step 3: Preview cleanup
./spacemate clean ~/Downloads --dry-run

# Step 4: Clean it up
./spacemate clean ~/Downloads
```

#### Example 2: Find Large Old Files

```bash
# Find files over 500MB, older than 90 days
./spacemate analyze /home/user \
    --min-size 500M \
    --older-than 90 \
    --output old-large-files.txt
```

#### Example 3: Scripted Cleanup

```bash
#!/bin/bash
# Automated cleanup script

# Clean temp directories (no confirmation)
./spacemate clean /tmp --force --no-backup

# Clean user downloads (with backup)
./spacemate clean ~/Downloads --older-than 180 --force

# Generate weekly report
./spacemate analyze /home --output ~/reports/disk-report-$(date +%Y%m%d).txt
```

---

## 🏗️ Architecture

### High-Level Design

```
┌─────────────────────────────────────────────────────────────┐
│                   Command Line Interface                     │
│              (Argument Parsing & User Interaction)          │
└────────────────────────┬────────────────────────────────────┘
                         │
         ┌───────────────┴──────────────┐
         │                              │
┌────────▼──────────┐          ┌───────▼────────┐
│   Disk Monitor    │          │ File Analyzer  │
│   - statvfs()     │          │ - opendir()    │
│   - stat()        │          │ - readdir()    │
│   - df utility    │          │ - stat()       │
└────────┬──────────┘          │ - MD5/SHA256   │
         │                     └────────┬────────┘
         │                              │
         │     ┌────────────────────────┘
         │     │
    ┌────▼─────▼─────┐
    │ Cleanup Engine  │
    │ - unlink()      │
    │ - Safety checks │
    │ - Logging       │
    └────────┬────────┘
             │
    ┌────────▼─────────┐
    │ Backup & Recovery │
    │ - copy_file()     │
    │ - Metadata store  │
    │ - Restore logic   │
    └───────────────────┘
```

### Module Breakdown

| Module | File | Responsibility | Key System Calls |
|--------|------|----------------|------------------|
| **Disk Monitor** | `disk_monitor.c` | Track filesystem statistics | `statvfs()`, `stat()` |
| **File Analyzer** | `file_analyzer.c` | Scan, categorize, detect duplicates | `opendir()`, `readdir()`, `stat()` |
| **Cleanup Engine** | `cleanup.c` | Safe file deletion | `unlink()`, `remove()` |
| **Backup System** | `backup.c` | Backup & restore operations | `open()`, `read()`, `write()` |
| **CLI Interface** | `main.c` | User interaction | `getopt()`, `printf()` |

### Key Algorithms

#### 1. Duplicate Detection
```
Algorithm: Hash-Based Duplicate Detection
Time Complexity: O(n log n)
Space Complexity: O(n)

1. Group files by size (quick filter)
2. For each size group:
   - Compute hash (MD5/SHA256) for each file
   - Group files by hash
3. Report groups with multiple files as duplicates
```

#### 2. Directory Scanning
```
Algorithm: Recursive DFS Traversal
Time Complexity: O(n) where n = number of files
Space Complexity: O(d) where d = max directory depth

1. Start from root directory
2. For each entry:
   - If file: collect metadata
   - If directory: recurse
3. Accumulate sizes bottom-up
```

#### 3. Safe Cleanup
```
Algorithm: Two-Phase Cleanup
Phase 1: Backup
  - Copy files to backup directory
  - Store metadata (path, size, timestamp)

Phase 2: Delete
  - Verify backup success
  - Delete original files
  - Log operations
  - On failure: rollback from backup
```

---

## 🧪 Testing

### Running Tests

```bash
# Run all unit tests
make test

# Run specific test suite
./tests/test_disk_monitor
./tests/test_file_analyzer
./tests/test_cleanup

# Run with memory leak detection
valgrind --leak-check=full ./spacemate scan /tmp
```

### Test Coverage

| Module | Unit Tests | Integration Tests | Coverage |
|--------|-----------|-------------------|----------|
| Disk Monitor | ✅ 12 tests | ✅ 3 tests | 94% |
| File Analyzer | ✅ 18 tests | ✅ 5 tests | 91% |
| Cleanup Engine | ✅ 15 tests | ✅ 4 tests | 88% |
| Backup System | ✅ 10 tests | ✅ 3 tests | 92% |

### Manual Testing Scenarios

```bash
# Test 1: Large directory scan
time ./spacemate scan /usr
# Expected: Complete in <10 seconds

# Test 2: Duplicate detection accuracy
mkdir test_dir
echo "test" > test_dir/file1.txt
cp test_dir/file1.txt test_dir/file2.txt
./spacemate analyze test_dir
# Expected: Detect 1 duplicate group

# Test 3: Backup and restore
./spacemate clean test_dir
./spacemate restore
# Expected: Files restored successfully
```

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

### Benchmarks

Tested on: Intel i5-1135G7, 16GB RAM, SSD

| Operation | Files | Time | Memory |
|-----------|-------|------|--------|
| Scan | 10,000 | 2.1s | 45 MB |
| Scan | 100,000 | 18.4s | 92 MB |
| Scan | 1,000,000 | 3m 12s | 184 MB |
| Duplicate Detection | 10,000 | 4.8s | 68 MB |
| Cleanup | 1,000 | 1.2s | 25 MB |

### Optimization Techniques

- Early size filtering before hashing (10x speedup)
- Memory-mapped file reading for large files
- Parallel directory traversal (future enhancement)
- Incremental hashing for resumable operations

---

## 🔧 Troubleshooting

### Common Issues

**Issue**: Permission denied errors
```bash
# Solution: Run with appropriate permissions
./spacemate scan /home/user  # Works
./spacemate scan /root       # Needs sudo
```

**Issue**: Slow scanning on network drives
```bash
# Solution: Exclude network mounts
./spacemate scan / --exclude /mnt,/media
```

**Issue**: Out of memory on large scans
```bash
# Solution: Scan subdirectories separately
./spacemate scan /home/user/Documents
./spacemate scan /home/user/Downloads
```

### Debug Mode

```bash
# Enable verbose output
./spacemate scan /path --verbose --debug

# Check logs
cat ~/.spacemate/logs/spacemate.log
```

### Getting Help

- 📧 Email: jmanas275@gmail.com
- 🐛 Issues: [GitHub Issues](https://github.com/team-raptors/spacemate/issues)
- 📖 Wiki: [Project Wiki](https://github.com/team-raptors/spacemate/wiki)
- 💬 Discussions: [GitHub Discussions](https://github.com/team-raptors/spacemate/discussions)

---

## 📚 Documentation

- **[User Guide](docs/USER_GUIDE.md)** - Comprehensive usage instructions
- **[API Reference](docs/API.md)** - Function documentation
- **[Architecture](docs/ARCHITECTURE.md)** - Detailed system design
- **[Contributing](CONTRIBUTING.md)** - Contribution guidelines
- **[Changelog](CHANGELOG.md)** - Version history

---

## 🗺️ Roadmap

### Phase 1: Core Functionality (✅ Complete)
- [x] Disk usage monitoring
- [x] Directory scanning
- [x] Duplicate detection
- [x] Safe cleanup
- [x] Backup and restore

### Phase 2: Enhanced Features (🚧 In Progress)
- [ ] GUI interface (Qt/GTK)
- [ ] Scheduled cleanup tasks
- [ ] Cloud storage integration
- [ ] Advanced filtering options
- [ ] Configuration profiles

### Phase 3: Advanced Capabilities (📋 Planned)
- [ ] Machine learning for smart recommendations
- [ ] Disk defragmentation
- [ ] Network drive support
- [ ] Multi-user support
- [ ] Plugin system

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
