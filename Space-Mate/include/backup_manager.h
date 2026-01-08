#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include <string>
#include <vector>

struct BackupEntry {
    std::string originalPath;
    std::string backupPath;
    unsigned long long size;
    std::string timestamp;
};

class BackupManager {
public:
    std::string createBackup(const std::string& filepath);
    void restoreFiles();
    void showBackups();
    
private:
    std::string getBackupDir();
    std::vector<BackupEntry> loadBackupIndex();
    bool copyFile(const std::string& source, const std::string& dest);
};

#endif