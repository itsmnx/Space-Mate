#ifndef CLEANUP_MANAGER_H
#define CLEANUP_MANAGER_H

#include <string>
#include <vector>
#include "file_analyzer.h"

class CleanupManager {
public:
    void cleanPath(const std::string& path, bool dryRun, bool force, bool verbose);
    void deleteFiles(const std::vector<FileInfo>& files, bool dryRun, bool force);
    
private:
    bool confirmDeletion(int fileCount, unsigned long long totalSize);
    void logOperation(const std::string& operation, const std::string& path);
};

#endif