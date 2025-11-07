#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <ctime>

namespace Utils {
    std::string formatSize(unsigned long long bytes);
    std::string getCurrentTimestamp();
    bool fileExists(const std::string& path);
    bool isDirectory(const std::string& path);
    unsigned long long getFileSize(const std::string& path);
    time_t getFileModTime(const std::string& path);
    void createDirectory(const std::string& path);
    std::string getHomeDir();
    std::string getTempDir();
    std::string getCacheDir();
}

#endif
