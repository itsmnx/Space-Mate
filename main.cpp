// ============================================================================
// FILE: main.cpp
// SpaceMate - Smart Disk Manager
// Entry Point and CLI Interface
// ============================================================================

#include "include/disk_monitor.h"
#include "include/file_analyzer.h"
#include "include/cleanup_manager.h"
#include "include/backup_manager.h"
#include "include/utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;

// ANSI Color Codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

void printBanner() {
    cout << CYAN << BOLD;
    cout << "╔════════════════════════════════════════╗\n";
    cout << "║     SpaceMate - Smart Disk Manager     ║\n";
    cout << "║         (Command Line Interface)       ║\n";
    cout << "╚════════════════════════════════════════╝\n";
    cout << RESET << "\n";
}

void printHelp() {
    cout << BOLD << "Usage:" << RESET << " ./spacemate <command> <path> [options]\n\n";
    cout << BOLD << "Commands:\n" << RESET;
    cout << "  scan <path>       - Scan disk usage and show statistics\n";
    cout << "  analyze <path>    - Analyze files (duplicates, temp files, old files)\n";
    cout << "  clean <path>      - Clean up unnecessary files\n";
    cout << "  restore           - Restore backed up files\n";
    cout << "  help              - Show this help message\n\n";
    cout << BOLD << "Options:\n" << RESET;
    cout << "  --dry-run         - Preview cleanup without making changes\n";
    cout << "  --verbose         - Show detailed output\n";
    cout << "  --force           - Skip confirmations (use with caution)\n\n";
    cout << BOLD << "Examples:\n" << RESET;
    cout << "  ./spacemate scan ~/Downloads\n";
    cout << "  ./spacemate analyze ~/Documents --verbose\n";
    cout << "  ./spacemate clean ~/temp --dry-run\n";
    cout << "  ./spacemate restore\n\n";
}

int main(int argc, char* argv[]) {
    printBanner();
    
    if (argc < 2) {
        printHelp();
        return 1;
    }
    
    string command = argv[1];
    string path = argc > 2 ? argv[2] : ".";
    bool dryRun = false;
    bool verbose = false;
    bool force = false;
    
    // Parse options
    for (int i = 3; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--dry-run") dryRun = true;
        else if (arg == "--verbose") verbose = true;
        else if (arg == "--force") force = true;
    }
    
    try {
        if (command == "help" || command == "--help" || command == "-h") {
            printHelp();
        }
        else if (command == "scan") {
            cout << BLUE << "📊 Scanning: " << RESET << path << "\n\n";
            DiskMonitor monitor;
            monitor.scanPath(path, verbose);
        }
        else if (command == "analyze") {
            cout << BLUE << "🔍 Analyzing: " << RESET << path << "\n\n";
            FileAnalyzer analyzer;
            analyzer.analyzePath(path, verbose);
        }
        else if (command == "clean") {
            if (dryRun) {
                cout << YELLOW << "🔍 DRY RUN MODE - No files will be deleted\n" << RESET;
            }
            cout << BLUE << "🧹 Cleaning: " << RESET << path << "\n\n";
            
            CleanupManager cleaner;
            cleaner.cleanPath(path, dryRun, force, verbose);
        }
        else if (command == "restore") {
            cout << BLUE << "📦 Restore Manager\n" << RESET << "\n";
            BackupManager backup;
            backup.showBackups();
            backup.restoreFiles();
        }
        else {
            cout << RED << "Error: Unknown command '" << command << "'\n" << RESET;
            cout << "Run './spacemate help' for usage information.\n";
            return 1;
        }
        
        cout << "\n" << GREEN << "✓ Operation completed successfully!\n" << RESET;
        
    } catch (const exception& e) {
        cout << RED << "Error: " << e.what() << RESET << "\n";
        return 1;
    }
    
    return 0;
}

