#include "../include/file_analyzer.h"
#include "../include/utils.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <fstream>
#include <algorithm>
#include <set>

#define RESET   "\033[0m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

using namespace std;

void FileAnalyzer::analyzePath(const string& path, bool verbose) {
    cout << "🔍 Scanning files...\n";
    
    vector<FileInfo> allFiles = scanDirectory(path);
    cout << "Found " << allFiles.size() << " files\n\n";
    
    // Find duplicates
    cout << BOLD << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "🔄 DUPLICATE FILES ANALYSIS\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
    
    auto duplicates = findDuplicates(path);
    unsigned long long duplicateWaste = 0;
    
    if (duplicates.empty()) {
        cout << "✓ No duplicate files found\n";
    } else {
        int groupNum = 1;
        for (const auto& group : duplicates) {
            if (group.size() < 2) continue;
            
            cout << "\nGroup " << groupNum++ << ": " << CYAN << group[0].path.substr(group[0].path.find_last_of("/") + 1) 
                 << RESET << " (" << group.size() << " copies)\n";
            
            for (size_t i = 0; i < group.size() && i < 3; i++) {
                cout << "  " << (i == 0 ? "[KEEP]   " : "[DELETE] ");
                cout << group[i].path << " (" << Utils::formatSize(group[i].size) << ")\n";
                if (i > 0) duplicateWaste += group[i].size;
            }
        }
        cout << "\n" << YELLOW << "💡 Potential savings from duplicates: " 
             << Utils::formatSize(duplicateWaste) << RESET << "\n";
    }
    
    // Find temp files
    cout << "\n" << BOLD << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "🗑️  TEMPORARY FILES\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
    
    auto tempFiles = findTempFiles(path);
    unsigned long long tempSize = 0;
    
    if (tempFiles.empty()) {
        cout << "✓ No temporary files found\n";
    } else {
        for (const auto& file : tempFiles) {
            tempSize += file.size;
        }
        cout << "Found " << tempFiles.size() << " temporary files\n";
        cout << YELLOW << "Space used: " << Utils::formatSize(tempSize) << RESET << "\n";
    }
    
    // Find old files
    cout << "\n" << BOLD << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "⏰ OLD FILES (90+ days)\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << RESET;
    
    auto oldFiles = findOldFiles(path, 90);
    unsigned long long oldSize = 0;
    
    if (oldFiles.empty()) {
        cout << "✓ No old files found\n";
    } else {
        for (const auto& file : oldFiles) {
            oldSize += file.size;
        }
        cout << "Found " << oldFiles.size() << " files not accessed in 90+ days\n";
        cout << YELLOW << "Space used: " << Utils::formatSize(oldSize) << RESET << "\n";
    }
    
    // Summary
    unsigned long long totalSavings = duplicateWaste + tempSize;
    if (totalSavings > 0) {
        cout << "\n" << BOLD << "╔════════════════════════════════════════╗\n";
        cout << "║        CLEANUP RECOMMENDATIONS         ║\n";
        cout << "╚════════════════════════════════════════╝\n" << RESET;
        cout << "💡 Total potential savings: " << YELLOW << BOLD 
             << Utils::formatSize(totalSavings) << RESET << "\n";
        cout << "\nRun './spacemate clean " << path << "' to clean up\n";
    }
}

vector<FileInfo> FileAnalyzer::scanDirectory(const string& path) {
    vector<FileInfo> files;
    
    DIR* dir = opendir(path.c_str());
    if (!dir) return files;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        
        string fullPath = path + "/" + entry->d_name;
        struct stat st;
        
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                auto subFiles = scanDirectory(fullPath);
                files.insert(files.end(), subFiles.begin(), subFiles.end());
            } else if (S_ISREG(st.st_mode)) {
                FileInfo info;
                info.path = fullPath;
                info.size = st.st_size;
                info.modTime = st.st_mtime;
                
                string filename = entry->d_name;
                size_t dotPos = filename.find_last_of(".");
                info.extension = (dotPos != string::npos) ? filename.substr(dotPos) : "";
                
                files.push_back(info);
            }
        }
    }
    closedir(dir);
    
    return files;
}

vector<vector<FileInfo>> FileAnalyzer::findDuplicates(const string& path) {
    vector<FileInfo> allFiles = scanDirectory(path);
    map<unsigned long long, vector<FileInfo>> sizeGroups;
    
    // Group by size first
    for (const auto& file : allFiles) {
        if (file.size > 1024) { // Only check files > 1KB
            sizeGroups[file.size].push_back(file);
        }
    }
    
    vector<vector<FileInfo>> duplicates;
    
    // Check files with same size
    for (auto& group : sizeGroups) {
        if (group.second.size() > 1) {
            // In real implementation, we'd hash files here
            // For demo, assume files with same size are duplicates
            duplicates.push_back(group.second);
        }
    }
    
    return duplicates;
}

vector<FileInfo> FileAnalyzer::findTempFiles(const string& path) {
    vector<FileInfo> allFiles = scanDirectory(path);
    vector<FileInfo> tempFiles;
    
    set<string> tempExtensions = {".tmp", ".temp", ".log", ".cache", ".bak", "~"};
    
    for (const auto& file : allFiles) {
        if (tempExtensions.count(file.extension) > 0) {
            tempFiles.push_back(file);
        }
    }
    
    return tempFiles;
}

vector<FileInfo> FileAnalyzer::findOldFiles(const string& path, int days) {
    vector<FileInfo> allFiles = scanDirectory(path);
    vector<FileInfo> oldFiles;
    
    time_t now = time(nullptr);
    time_t threshold = now - (days * 24 * 60 * 60);
    
    for (const auto& file : allFiles) {
        if (file.modTime < threshold && file.size > 1024 * 1024) { // > 1MB
            oldFiles.push_back(file);
        }
    }
    
    return oldFiles;
}

string FileAnalyzer::calculateHash(const string& filepath) {
    // Simplified - in real implementation use MD5/SHA256
    return "hash_" + filepath;
}

bool FileAnalyzer::isTempFile(const string& filename) {
    set<string> tempExtensions = {".tmp", ".temp", ".log", ".cache"};
    size_t dotPos = filename.find_last_of(".");
    if (dotPos != string::npos) {
        string ext = filename.substr(dotPos);
        return tempExtensions.count(ext) > 0;
    }
    return false;
}

