#ifndef DISK_MONITOR_H
#define DISK_MONITOR_H

#include <string>
#include <sys/statvfs.h>

class DiskMonitor {
public:
    void scanPath(const std::string& path, bool verbose = false);
    void showDiskUsage(const std::string& path);
    void showLargestDirectories(const std::string& path, int limit = 5);
    
private:
    void printProgressBar(double percentage);
    std::string formatSize(unsigned long long bytes);
};

#endif