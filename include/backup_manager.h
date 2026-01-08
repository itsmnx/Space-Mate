#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

struct BackupEntry {
    std::string originalPath;
    std::string backupPath;
    unsigned long long size;
    std::string timestamp;
};

class BackupManager {
public:
    // ===== Existing CLI method =====
    std::string createBackup(const std::string& filepath);

    // ===== New method for GUI =====
    std::string createBackup(const std::string& source, const std::string& dest);

    // Optional alias for GUI calls if needed
    std::string createBackupGUI(const std::string& source, const std::string& dest);

    void restoreFiles();
    void showBackups();

    // Public so GUI can access backup directory path
    std::string getBackupDir();

    // Make loadBackupIndex public so GUI can access backup history
    std::vector<BackupEntry> loadBackupIndex();
    
    // Add backup entry to index
    void addBackupIndexEntry(const std::string& originalPath, const std::string& backupPath);

private:
    bool copyFile(const std::string& source, const std::string& dest);
};

#endif
