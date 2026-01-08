# SpaceMate - Running Instructions

## Build the Project

```bash
cd build
make -j4
```

---

## Running the CLI Application

### Basic Commands

**Show Help:**
```bash
./spacemate_cli help
```

**Scan Disk Usage:**
```bash
./spacemate_cli scan /path/to/directory
```

**Analyze Files (Find Duplicates, Temp Files, Old Files):**
```bash
./spacemate_cli analyze /path/to/directory
```

**Clean Up Files:**
```bash
./spacemate_cli clean /path/to/directory
```

**Restore Backed Up Files:**
```bash
./spacemate_cli restore
```

### CLI Options

**Dry Run (Preview without deleting):**
```bash
./spacemate_cli clean /path/to/directory --dry-run
```

**Verbose Output:**
```bash
./spacemate_cli analyze /path/to/directory --verbose
```

**Force (Skip Confirmations):**
```bash
./spacemate_cli clean /path/to/directory --force
```

**Combined Options:**
```bash
./spacemate_cli clean /path/to/directory --dry-run --verbose
```

### CLI Examples

**Scan Downloads Folder:**
```bash
./spacemate_cli scan ~/Downloads
```

**Analyze Documents Folder:**
```bash
./spacemate_cli analyze ~/Documents --verbose
```

**Clean Temp Folder (Preview Only):**
```bash
./spacemate_cli clean ~/temp --dry-run
```

---

## Running the GUI Application

### From WSL (Linux Environment)

**Basic Launch:**
```bash
export DISPLAY=:0
./SpacemateGUI
```

**Background Launch:**
```bash
export DISPLAY=:0
./SpacemateGUI &
```

### From Windows (PowerShell)

**Using WSL Command:**
```powershell
wsl -e bash -ic "cd /mnt/c/Users/jmana/OneDrive/Desktop/Spacemate/build && export DISPLAY=:0 && ./SpacemateGUI"
```

**Using Batch File (from project root):**
```powershell
.\spacemate.bat gui
```

---

## GUI Features

- **Dashboard Tab**: View disk usage with color-coded storage visualization
  - Blue: Documents (PDF, DOCX, TXT, etc.)
  - Green: Images (JPG, PNG, GIF, etc.)
  - Purple: Applications & Executables
  - Orange: System Files
  - Gray: Free Space

- **Analyze Tab**: Scan folders to find:
  - Duplicate files (with MD5 hash detection)
  - Temporary files
  - Old files (45+ days)

- **Cleanup Tab**: 
  - Review and delete duplicate files
  - Select individual files or use "Select All Duplicates"
  - Files are backed up before deletion

- **Backup Tab**:
  - View all backed-up files
  - Restore individual files
  - See backup timestamps and sizes

- **Activity Log**:
  - Real-time monitoring of folder statistics
  - Detailed file counts by category
  - Automatic updates while monitoring is active

---

## Prerequisites

### For GUI (X11 Display Server)
Make sure VcXsrv or another X11 server is running on Windows:
1. Launch VcXsrv (XLaunch)
2. Use "Multiple windows" mode
3. Display number: 0
4. Enable "Disable access control"

### For CLI
No special requirements - works directly in WSL terminal.

---

## Common Issues

**GUI doesn't show:**
- Check if VcXsrv is running
- Verify DISPLAY environment variable: `echo $DISPLAY`
- Try: `export DISPLAY=:0`

**Permission Warnings:**
- Harmless Qt warning about `/run/user/1000/` can be ignored
- Does not affect functionality

**Build Errors:**
- Run `make clean` then `make -j4` to rebuild
- Check that all dependencies are installed

---

## File Locations

- **CLI Executable**: `build/spacemate_cli` (295 KB)
- **GUI Executable**: `build/SpacemateGUI` (802 KB)
- **Backups**: `~/.spacemate/backups/`
- **Logs**: Project `logs/` directory
