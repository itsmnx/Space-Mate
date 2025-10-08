#include "../include/disk_monitor.h"
#include "../include/utils.h"
#include <iostream>
#include <iomanip>
#include <sys/statvfs.h>
#include <dirent.h>
#include <sys/stat.h>
#include <map>
#include <algorithm>

#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

using namespace std;

void DiskMonitor::scanPath(const string& path, bool verbose) {
    showDiskUsage(path);
    cout << "\n";
    showLargestDirectories(path);
}

void DiskMonitor::showDiskUsage(const string& path) {
    struct statvfs stat;
    
    if (statvfs(path.c_str(), &stat) != 0) {
        throw runtime_error("Failed to get disk statistics");
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
            // Simple size calculation (would need recursive in real implementation)
            dirSizes[entry->d_name] = st.st_size * 1000; // Approximation
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
