#ifndef SCAN_RESULTS_H
#define SCAN_RESULTS_H

#include <QString>
#include <vector>
#include <string>
#include <utility>

struct FileDetail {
    QString path;
    qint64 size;
    QString type;
    QString lastModified;
    bool isDuplicate;
    bool isOld;
    QString hash;
};

using ScanResults = std::vector<FileDetail>;
using DuplicateGroup = std::vector<FileDetail>;
using DuplicateGroups = std::vector<DuplicateGroup>;

#endif // SCAN_RESULTS_H