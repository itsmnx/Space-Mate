#include "../include/cleanup_manager.h"
#include "../include/backup_manager.h"
#include "../include/file_analyzer.h"
#include "../include/utils.h"
#include <iostream>
#include <fstream>
#include <unistd.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

using namespace std;

void CleanupManager::cleanPath(const string& path, bool dryRun, bool force, bool verbose) {
    FileAnalyzer analyzer;
    BackupManager backup;
    
    // Find files to clean
    vector<FileInfo> filesToDelete;
    
    cout << "Analyzing files for cleanup...\n";
    
    // Get temp files
    auto tempFiles = analyzer.findTempFiles(path);
    filesToDelete.insert(filesToDelete.end(), tempFiles.begin(), tempFiles.end());
    
    // Get duplicates (keep first, delete rest)
    auto duplicateGroups = analyzer.findDuplicates(path);
    for (const auto& group : duplicateGroups) {
        if (group.size() > 1) {
            for (size_t i = 1; i < group.size(); i++) {
                filesToDelete.push_back(group[i]);
            }
        }
    }
    
    if (filesToDelete.empty()) {
        cout << GREEN << "✓ No files need cleanup!\n" << RESET;
        return;
    }
    
    // Calculate total size
    unsigned long long totalSize = 0;
    for (const auto& file : filesToDelete) {
        totalSize += file.size;
    }
    
    cout << "\n" << BOLD << "Cleanup Summary:\n" << RESET;
    cout << "Files to delete: " << filesToDelete.size() << "\n";
    cout << "Space to free: " << YELLOW << Utils::formatSize(totalSize) << RESET << "\n\n";
    
    if (dryRun) {
        cout << YELLOW << "DRY RUN - Showing what would be deleted:\n" << RESET;
        int count = 0;
        for (const auto& file : filesToDelete) {
            if (count++ >= 10) {
                cout << "  ... and " << (filesToDelete.size() - 10) << " more files\n";
                break;
            }
            cout << "  ✗ " << file.path << " (" << Utils::formatSize(file.size) << ")\n";
        }
        return;
    }
    
    // Confirm deletion
    if (!force && !confirmDeletion(filesToDelete.size(), totalSize)) {
        cout << "Cleanup cancelled.\n";
        return;
    }
    
    // Create backup first
    if (!force) {
        cout << "\n🔒 Creating backup...\n";
        for (const auto& file : filesToDelete) {
            backup.createBackup(file.path);
        }
        cout << GREEN << "✓ Backup complete\n" << RESET;
    }
    
    // Delete files
    deleteFiles(filesToDelete, false, force);
}

void CleanupManager::deleteFiles(const vector<FileInfo>& files, bool dryRun, bool force) {
    int deleted = 0;
    unsigned long long totalFreed = 0;
    
    cout << "\n🗑️  Deleting files...\n";
    
    for (const auto& file : files) {
        if (unlink(file.path.c_str()) == 0) {
            deleted++;
            totalFreed += file.size;
            logOperation("DELETE", file.path);
            
            if (deleted % 10 == 0) {
                cout << "  Deleted " << deleted << "/" << files.size() << " files...\r" << flush;
            }
        }
    }
    
    cout << "\n" << GREEN << "✓ Deleted " << deleted << " files\n";
    cout << "✓ Freed " << Utils::formatSize(totalFreed) << " of space\n" << RESET;
}

bool CleanupManager::confirmDeletion(int fileCount, unsigned long long totalSize) {
    cout << YELLOW << "\n⚠️  Warning: About to delete " << fileCount << " files (" 
         << Utils::formatSize(totalSize) << ")\n" << RESET;
    cout << "Files will be backed up before deletion.\n";
    cout << "Continue? (y/N): ";
    
    string response;
    getline(cin, response);
    
    return (response == "y" || response == "Y" || response == "yes");
}

void CleanupManager::logOperation(const string& operation, const string& path) {
    string logDir = Utils::getHomeDir() + "/.spacemate/logs";
    Utils::createDirectory(Utils::getHomeDir() + "/.spacemate");
    Utils::createDirectory(logDir);
    
    string logFile = logDir + "/operations.log";
    ofstream log(logFile, ios::app);
    
    if (log.is_open()) {
        log << Utils::getCurrentTimestamp() << " | " << operation << " | " << path << "\n";
        log.close();
    }
}

// ===== GUI-specific methods =====
void CleanupManager::cleanTempFiles() {
    // Linux / WSL temp folder
    cleanPath("/tmp", false, true, true);

    // Windows WSL example (uncomment if needed)
    // cleanPath("/mnt/c/Users/jmana/AppData/Local/Temp", false, true, true);
}

void CleanupManager::cleanCache() {
    // Linux user cache folder
    cleanPath(Utils::getHomeDir() + "/.cache", false, true, true);

    // Windows WSL example (uncomment if needed)
    // cleanPath("/mnt/c/Users/jmana/AppData/Local/Packages/.../LocalCache", false, true, true);
}
