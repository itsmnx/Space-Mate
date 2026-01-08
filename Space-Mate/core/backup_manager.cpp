
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