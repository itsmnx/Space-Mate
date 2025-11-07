#ifndef CUSTOM_TYPES_H
#define CUSTOM_TYPES_H

#include <QMetaType>
#include <vector>
#include <string>
#include <utility>

// Define our custom type
using ScanResult = std::vector<std::pair<std::string, long long>>;

// Register it with Qt's meta-type system
Q_DECLARE_METATYPE(ScanResult)

#endif // CUSTOM_TYPES_H