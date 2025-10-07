// ============================================================================
// FILE: core/cleanup_manager.cpp
// Safe file cleanup implementation
// ============================================================================

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


// ============================================================================
// FILE: core/backup_manager.cpp
// Backup and restore functionality
// ============================================================================

#include "../include/backup_manager.h"
#include "../include/utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/stat.h>

#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

using namespace std;

string BackupManager::getBackupDir() {
    string backupDir = Utils::getHomeDir() + "/.spacemate/backup";
    Utils::createDirectory(Utils::getHomeDir() + "/.spacemate");
    Utils::createDirectory(backupDir);
    return backupDir;
}

string BackupManager::createBackup(const string& filepath) {
    if (!Utils::fileExists(filepath)) {
        return "";
    }
    
    string backupDir = getBackupDir() + "/" + Utils::getCurrentTimestamp();
    Utils::createDirectory(backupDir);
    
    // Get filename from path
    size_t lastSlash = filepath.find_last_of("/");
    string filename = (lastSlash != string::npos) ? filepath.substr(lastSlash + 1) : filepath;
    
    string backupPath = backupDir + "/" + filename;
    
    if (copyFile(filepath, backupPath)) {
        // Save to index
        string indexFile = getBackupDir() + "/index.txt";
        ofstream index(indexFile, ios::app);
        if (index.is_open()) {
            index << Utils::getCurrentTimestamp() << "|"
                  << filepath << "|"
                  << backupPath << "|"
                  << Utils::getFileSize(filepath) << "\n";
            index.close();
        }
        return backupPath;
    }
    
    return "";
}

void BackupManager::showBackups() {
    vector<BackupEntry> backups = loadBackupIndex();
    
    if (backups.empty()) {
        cout << "No backups found.\n";
        return;
    }
    
    cout << BOLD << "Available Backups:\n" << RESET;
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    for (size_t i = 0; i < backups.size(); i++) {
        cout << "[" << (i + 1) << "] " << CYAN << backups[i].timestamp << RESET << "\n";
        cout << "    Original: " << backups[i].originalPath << "\n";
        cout << "    Size: " << Utils::formatSize(backups[i].size) << "\n";
    }
}

void BackupManager::restoreFiles() {
    vector<BackupEntry> backups = loadBackupIndex();
    
    if (backups.empty()) {
        cout << "No backups available to restore.\n";
        return;
    }
    
    cout << "\nEnter backup number to restore (0 to cancel): ";
    int choice;
    cin >> choice;
    cin.ignore();
    
    if (choice < 1 || choice > (int)backups.size()) {
        cout << "Restore cancelled.\n";
        return;
    }
    
    BackupEntry& entry = backups[choice - 1];
    
    cout << "\nRestoring: " << entry.originalPath << "\n";
    
    if (Utils::fileExists(entry.originalPath)) {
        cout << YELLOW << "Warning: File already exists. Overwrite? (y/N): " << RESET;
        string response;
        getline(cin, response);
        if (response != "y" && response != "Y") {
            cout << "Restore cancelled.\n";
            return;
        }
    }
    
    if (copyFile(entry.backupPath, entry.originalPath)) {
        cout << GREEN << "✓ File restored successfully!\n" << RESET;
    } else {
        cout << "\033[31mError: Failed to restore file\n" << RESET;
    }
}

vector<BackupEntry> BackupManager::loadBackupIndex() {
    vector<BackupEntry> backups;
    string indexFile = getBackupDir() + "/index.txt";
    
    ifstream index(indexFile);
    if (!index.is_open()) {
        return backups;
    }
    
    string line;
    while (getline(index, line)) {
        BackupEntry entry;
        stringstream ss(line);
        string sizeStr;
        
        getline(ss, entry.timestamp, '|');
        getline(ss, entry.originalPath, '|');
        getline(ss, entry.backupPath, '|');
        getline(ss, sizeStr, '|');
        
        entry.size = stoull(sizeStr);
        backups.push_back(entry);
    }
    
    index.close();
    return backups;
}

bool BackupManager::copyFile(const string& source, const string& dest) {
    ifstream src(source, ios::binary);
    if (!src.is_open()) return false;
    
    ofstream dst(dest, ios::binary);
    if (!dst.is_open()) {
        src.close();
        return false;
    }
    
    dst << src.rdbuf();
    
    src.close();
    dst.close();
    
    return true;
}