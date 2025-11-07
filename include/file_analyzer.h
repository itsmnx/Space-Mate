#ifndef FILE_ANALYZER_H
#define FILE_ANALYZER_H

#include <string>
#include <vector>
#include <map>

struct FileInfo {
    std::string path;
    unsigned long long size;
    time_t modTime;
    std::string hash;
    std::string extension;
};

class FileAnalyzer {
public:
    // ===== Existing CLI methods =====
    void analyzePath(const std::string& path, bool verbose = false);
    std::vector<std::vector<FileInfo>> findDuplicates(const std::string& path);
    std::vector<FileInfo> findTempFiles(const std::string& path);
    std::vector<FileInfo> findOldFiles(const std::string& path, int days = 90);

    // ===== GUI-friendly wrappers =====
    std::vector<std::vector<FileInfo>> getDuplicateGroups(const std::string& path);
    std::vector<FileInfo> getTempFiles(const std::string& path);
    std::vector<FileInfo> getOldFiles(const std::string& path, int days = 90);
    unsigned long long getPotentialSavings(const std::string& path);

    // Optional helper counts
    int countTempFiles(const std::string& path);
    int countOldFiles(const std::string& path, int days = 90);

private:
    std::vector<FileInfo> scanDirectory(const std::string& path);
    std::string calculateHash(const std::string& filepath);
    bool isTempFile(const std::string& filename);
};

#endif
