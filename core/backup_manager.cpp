#include "../include/backup_manager.h"
#include "../include/utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <filesystem>

#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define BOLD    "\033[1m"

using namespace std;
namespace fs = std::filesystem;

// ===== Helper: get backup directory =====
string BackupManager::getBackupDir() {
    string baseDir = Utils::getHomeDir() + "/.spacemate";
    Utils::createDirectory(baseDir);
    string backupDir = baseDir + "/backup";
    Utils::createDirectory(backupDir);
    return backupDir;
}

// ===== CLI method: create backup for a single file =====
string BackupManager::createBackup(const string& filepath) {
    if (!Utils::fileExists(filepath)) return "";

    string backupDir = getBackupDir() + "/" + Utils::getCurrentTimestamp();
    Utils::createDirectory(backupDir);

    string filename = filepath.substr(filepath.find_last_of("/")+1);
    string backupPath = backupDir + "/" + filename;

    if (copyFile(filepath, backupPath)) {
        // Save entry to index
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

// ===== GUI method: create backup from source to destination =====
// Returns destination path on success, "" on failure
string BackupManager::createBackup(const string& source, const string& dest) {
    try {
        if (!fs::exists(source)) return "";

        if (fs::exists(dest)) fs::remove_all(dest);
        fs::create_directories(dest);
        fs::copy(source, dest, fs::copy_options::recursive);

    } catch (const exception& e) {
        cerr << "Backup failed: " << e.what() << endl;
        return "";
    }

    return dest;
}

// ===== Show CLI backups =====
void BackupManager::showBackups() {
    vector<BackupEntry> backups = loadBackupIndex();
    if (backups.empty()) {
        cout << "No backups found.\n";
        return;
    }

    cout << BOLD << "Available Backups:\n" << RESET;
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    for (size_t i = 0; i < backups.size(); i++) {
        cout << "[" << (i+1) << "] " << CYAN << backups[i].timestamp << RESET << "\n";
        cout << "    Original: " << backups[i].originalPath << "\n";
        cout << "    Size: " << Utils::formatSize(backups[i].size) << "\n";
    }
}

// ===== Restore file from backup =====
void BackupManager::restoreFiles() {
    vector<BackupEntry> backups = loadBackupIndex();
    if (backups.empty()) {
        cout << "No backups available to restore.\n";
        return;
    }

    cout << "\nEnter backup number to restore (0 to cancel): ";
    int choice; cin >> choice; cin.ignore();

    if (choice < 1 || choice > (int)backups.size()) {
        cout << "Restore cancelled.\n";
        return;
    }

    BackupEntry& entry = backups[choice-1];

    if (Utils::fileExists(entry.originalPath)) {
        cout << YELLOW << "Warning: File exists. Overwrite? (y/N): " << RESET;
        string resp; getline(cin, resp);
        if (resp != "y" && resp != "Y") { cout << "Restore cancelled.\n"; return; }
    }

    if (copyFile(entry.backupPath, entry.originalPath))
        cout << GREEN << "✓ File restored successfully!\n" << RESET;
    else
        cout << "\033[31mError: Failed to restore file\n" << RESET;
}

// ===== Load backup index =====
vector<BackupEntry> BackupManager::loadBackupIndex() {
    vector<BackupEntry> backups;
    string indexFile = getBackupDir() + "/index.txt";
    ifstream index(indexFile);
    if (!index.is_open()) return backups;

    string line;
    while (getline(index, line)) {
        BackupEntry entry;
        stringstream ss(line); string sizeStr;
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

// ===== Add backup entry to index =====
void BackupManager::addBackupIndexEntry(const string& originalPath, const string& backupPath) {
    string indexFile = getBackupDir() + "/index.txt";
    ofstream index(indexFile, ios::app);
    if (index.is_open()) {
        unsigned long long size = 0;
        try {
            if (fs::exists(backupPath)) {
                size = fs::file_size(backupPath);
            }
        } catch (...) {
            size = 0;
        }
        
        index << Utils::getCurrentTimestamp() << "|"
              << originalPath << "|"
              << backupPath << "|"
              << size << "\n";
        index.close();
    }
}

// ===== Copy file helper =====
bool BackupManager::copyFile(const string& source, const string& dest) {
    ifstream src(source, ios::binary); if (!src.is_open()) return false;
    ofstream dst(dest, ios::binary); if (!dst.is_open()) { src.close(); return false; }
    dst << src.rdbuf();
    src.close(); dst.close();
    return true;
}
