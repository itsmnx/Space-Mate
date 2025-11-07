#ifndef DISK_MONITOR_H
#define DISK_MONITOR_H

#include <string>
#include <sys/statvfs.h>
#include <vector>
#include <utility> // for std::pair
#include <thread>  // for optional background monitoring
#include <atomic>  // for thread-safe monitoring flag

class DiskMonitor {
public:
    // ===== Existing CLI methods =====
    void scanPath(const std::string& path, bool verbose = false);
    void showDiskUsage(const std::string& path);
    void showLargestDirectories(const std::string& path, int limit = 5);

    // ===== GUI-friendly methods =====
    std::vector<std::pair<std::string, long long>> getLargestDirectories(const std::string& path, int limit = 5);
    std::vector<std::pair<std::string, long long>> getDiskInfo(const std::string& path);

    // Optional monitoring GUI features
    void startMonitoring(const std::string& path = "/"); // monitor a specific path
    void stopMonitoring();
    bool isMonitoring() const { return monitoring; }

private:
    void printProgressBar(double percentage);
    std::string formatSize(unsigned long long bytes);

    // Internal GUI flags
    std::atomic<bool> monitoring{false};   // atomic for thread safety
    std::thread monitorThread;             // optional background thread
    std::string monitoredPath;             // path currently being monitored

    void monitorLoop();                    // function for periodic updates
};

#endif
