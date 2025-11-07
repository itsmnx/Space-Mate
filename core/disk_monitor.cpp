#include "../include/disk_monitor.h"
#include "../include/utils.h"
#include <iostream>
#include <iomanip>
#include <sys/statvfs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <map>
#include <algorithm>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>

#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

using namespace std;

// ================= CLI FUNCTIONS =================

void DiskMonitor::scanPath(const string& path, bool verbose) {
    showDiskUsage(path);
    cout << "\n";
    showLargestDirectories(path);
}

void DiskMonitor::showDiskUsage(const string& path) {
    struct statvfs stat;
    
    if (statvfs(path.c_str(), &stat) != 0) {
        cerr << "\n⚠️  Warning: Cannot get disk statistics for " << path << "\n";
        return;
    }

    unsigned long long total = stat.f_blocks * stat.f_frsize;
    unsigned long long available = stat.f_bavail * stat.f_frsize;
    unsigned long long used = total - available;
    double percentage = (used * 100.0) / total;
    
    cout << BOLD << "╔════════════════════════════════════════╗\n";
    cout << "║        DISK USAGE STATISTICS          ║\n";
    cout << "╚════════════════════════════════════════╝\n" << RESET;
    
    cout << "\nFilesystem: " << path << "\n";
    cout << "Total Size:     " << formatSize(total) << "\n";
    cout << "Used Space:     " << formatSize(used) << "  ";
    printProgressBar(percentage);
    cout << " " << fixed << setprecision(1) << percentage << "%\n";
    cout << "Free Space:     " << formatSize(available) << "\n";
    
    if (percentage > 75) {
        cout << YELLOW << "\n⚠️  WARNING: Disk usage above 75% - cleanup recommended\n" << RESET;
    } else {
        cout << GREEN << "\n✓ Disk usage is healthy\n" << RESET;
    }
}

void DiskMonitor::showLargestDirectories(const string& path, int limit) {
    cout << BOLD << "\nTop " << limit << " Largest Directories:\n" << RESET;
    
    map<string, unsigned long long> dirSizes;
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        cout << "  Unable to scan directories\n";
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        
        string fullPath = path + "/" + entry->d_name;
        struct stat st;
        
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            // Approximation: actual size calculation can be recursive
            dirSizes[entry->d_name] = st.st_size * 1000;
        }
    }
    closedir(dir);
    
    vector<pair<string, unsigned long long>> sortedDirs(dirSizes.begin(), dirSizes.end());
    sort(sortedDirs.begin(), sortedDirs.end(), 
         [](const auto& a, const auto& b) { return a.second > b.second; });
    
    int count = 0;
    for (const auto& item : sortedDirs) {
        if (count++ >= limit) break;
        cout << "  " << (count) << ". " << CYAN << item.first << "/" << RESET;
        cout << string(30 - item.first.length(), ' ');
        cout << formatSize(item.second) << "\n";
    }
}

void DiskMonitor::printProgressBar(double percentage) {
    int barWidth = 20;
    int filled = (int)(barWidth * percentage / 100.0);
    
    cout << "[";
    for (int i = 0; i < barWidth; i++) {
        if (i < filled) cout << "█";
        else cout << "░";
    }
    cout << "]";
}

string DiskMonitor::formatSize(unsigned long long bytes) {
    return Utils::formatSize(bytes);
}

// ================= GUI FUNCTIONS =================

    // Return structured disk info: Total, Used, Free
vector<pair<string, long long>> DiskMonitor::getDiskInfo(const string& path) {
    vector<pair<string, long long>> info;
    struct statvfs stat;
    
    // Convert Windows path to WSL path if needed
    string wslPath = path;
    if (path.length() >= 2 && path[1] == ':') {
        // This is a Windows path, convert to WSL path
        wslPath = "/mnt/" + string(1, tolower(path[0])) + path.substr(2);
        std::replace(wslPath.begin(), wslPath.end(), '\\', '/');
    }

    if (statvfs(wslPath.c_str(), &stat) != 0) {
        cerr << "Failed to get disk info for path: " << wslPath << endl;
        return info;
    }

    unsigned long long total = stat.f_blocks * stat.f_frsize;
    unsigned long long free  = stat.f_bfree * stat.f_frsize;
    unsigned long long used  = total - free;    info.push_back({"Total", (long long)total});
    info.push_back({"Used",  (long long)used});
    info.push_back({"Free",  (long long)free});

    return info;
}

// Optionally: Return top N largest directories for GUI
vector<pair<string, long long>> DiskMonitor::getLargestDirectories(const string& path, int limit) {
    map<string, unsigned long long> dirSizes;
    
    // Convert Windows path to WSL path if needed
    string wslPath = path;
    if (path.length() >= 2 && path[1] == ':') {
        // This is a Windows path, convert to WSL path
        wslPath = "/mnt/" + string(1, tolower(path[0])) + path.substr(2);
        std::replace(wslPath.begin(), wslPath.end(), '\\', '/');
    }
    
    DIR* dir = opendir(wslPath.c_str());
    if (!dir) {
        cerr << "Failed to open directory: " << wslPath << endl;
        return {};
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        string fullPath = path + "/" + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            dirSizes[entry->d_name] = st.st_size * 1000;
        }
    }
    closedir(dir);
    
    vector<pair<string, long long>> sortedDirs(dirSizes.begin(), dirSizes.end());
    sort(sortedDirs.begin(), sortedDirs.end(),
         [](const auto& a, const auto& b) { return a.second > b.second; });
    
    if (sortedDirs.size() > (size_t)limit)
        sortedDirs.resize(limit);
    
    return sortedDirs;
}

void DiskMonitor::monitorLoop() {
    while (monitoring) {
        // You can update GUI via signals or polling
        auto info = getDiskInfo(monitoredPath);
        auto dirs = getLargestDirectories(monitoredPath, 5);

        // Sleep for a while (e.g., update every 5 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void DiskMonitor::startMonitoring(const std::string& path) {
    if (monitoring) return;  // already running

    monitoredPath = path;
    monitoring = true;
    monitorThread = std::thread(&DiskMonitor::monitorLoop, this);
    monitorThread.detach(); // or join in destructor if you prefer
}

void DiskMonitor::stopMonitoring() {
    if (!monitoring) return;

    monitoring = false;
    // If detached thread, it will exit naturally after the next sleep
    // Otherwise, you can join here if needed
}