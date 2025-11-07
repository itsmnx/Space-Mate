#include "../include/utils.h"
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <pwd.h>
#include <cstdlib>  // for getenv

using namespace std;

namespace Utils {

// Existing functions ...

string formatSize(unsigned long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    
    ostringstream oss;
    oss << fixed << setprecision(2) << size << " " << units[unit];
    return oss.str();
}

string getCurrentTimestamp() {
    time_t now = time(nullptr);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", localtime(&now));
    return string(buf);
}

bool fileExists(const string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool isDirectory(const string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

unsigned long long getFileSize(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

time_t getFileModTime(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

void createDirectory(const string& path) {
    mkdir(path.c_str(), 0755);
}

string getHomeDir() {
    const char* home = getenv("HOME");
    if (home) return string(home);
    
    struct passwd* pw = getpwuid(getuid());
    if (pw) return string(pw->pw_dir);
    
    return "/tmp";
}

// ===== Add GUI-friendly functions here =====
string getTempDir() {
#ifdef _WIN32
    return string(getenv("TEMP"));
#else
    return "/tmp";
#endif
}

string getCacheDir() {
#ifdef _WIN32
    return string(getenv("LOCALAPPDATA")) + "\\Cache";
#else
    return getHomeDir() + "/.cache";
#endif
}

}  // namespace Utils
