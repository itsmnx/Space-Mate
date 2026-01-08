
#include "mainwindow.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QDateTime>
#include <QFileDialog>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <QTimer>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QStorageInfo>
#include <QCheckBox>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QCryptographicHash>
#include <unordered_map>
#include <QSet>
#include <QTextStream>
#include <QIODevice>

namespace fs = std::filesystem;

// ==================== ScanWorker ====================
ScanWorker::ScanWorker(const std::string &path, QObject *parent)
    : QThread(parent), scanPath(path) {}

void ScanWorker::run() {
    try {
        emit scanProgress(0);
        ScanResults results;
        std::map<QString, std::vector<FileDetail>> hashToFiles;
        
        int fileCount = 0;
        long long totalSize = 0;
        QDateTime now = QDateTime::currentDateTime();
        QDateTime oldThreshold = now.addDays(-90);

        for (const auto &entry : fs::recursive_directory_iterator(scanPath, 
                                   fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                try {
                    FileDetail detail;
                    detail.path = QString::fromStdString(entry.path().string());
                    detail.size = entry.file_size();
                    
                    // Fix for last_write_time conversion
                    auto ftime = entry.last_write_time();
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                    ftime - fs::file_time_type::clock::now()
                                    + std::chrono::system_clock::now());
                    detail.lastModified = QDateTime::fromSecsSinceEpoch(
                        std::chrono::system_clock::to_time_t(sctp)).toString("yyyy-MM-dd hh:mm:ss");
                    
                    detail.type = QString::fromStdString(entry.path().extension().string());
                    detail.isDuplicate = false;
                    detail.isOld = QDateTime::fromSecsSinceEpoch(
                        std::chrono::system_clock::to_time_t(sctp)) < oldThreshold;
                    
                    // Calculate file hash for duplicate detection
                    if (detail.size > 1024) {
                        QFile file(detail.path);
                        if (file.open(QFile::ReadOnly)) {
                            QCryptographicHash hash(QCryptographicHash::Md5);
                            hash.addData(&file);
                            detail.hash = QString(hash.result().toHex());
                            hashToFiles[detail.hash].push_back(detail);
                            file.close();
                        }
                    }
                    
                    results.push_back(detail);
                    fileCount++;
                    totalSize += detail.size;
                    
                    // Dynamic progress update
                    if (fileCount % 50 == 0) {
                        int progress = std::min(90, 10 + (fileCount / 10));
                        emit scanProgress(progress);
                    }
                } catch (const std::exception &e) {
                    continue;
                }
            }
        }

        // Find duplicates
        DuplicateGroups duplicateGroups;
        for (const auto& hashGroup : hashToFiles) {
            if (hashGroup.second.size() > 1) {
                duplicateGroups.push_back(hashGroup.second);
                for (auto& file : results) {
                    if (file.hash == hashGroup.first) {
                        file.isDuplicate = true;
                    }
                }
            }
        }

        emit scanProgress(100);
        emit scanComplete(results, duplicateGroups);
    } catch (const std::exception &e) {
        emit scanError(QString::fromStdString(e.what()));
    }
}

// ==================== MainWindow ====================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isMonitoring(false), isScanning(false), scanWorker(nullptr), lastScannedPath("/")
{
    qRegisterMetaType<FileDetail>();
    qRegisterMetaType<ScanResults>();
    qRegisterMetaType<DuplicateGroups>();

    backupManager = std::make_unique<BackupManager>();
    cleanupManager = std::make_unique<CleanupManager>();
    diskMonitor = std::make_unique<DiskMonitor>();
    fileAnalyzer = std::make_unique<FileAnalyzer>();

    setupUI();
    setupConnections();
    
    // Delay initial update to ensure UI is ready
    QTimer::singleShot(1000, this, [this]() {
    addLog("=== DELAYED INITIAL DISK INFO UPDATE ===", "INFO");
    updateDiskInfo();
});

    setWindowTitle("Spacemate - Disk Space Manager");
    resize(1200, 800);
}

void MainWindow::setupConnections() {
    connect(startScanBtn, &QPushButton::clicked, this, &MainWindow::startScan);
    connect(browseScanBtn, &QPushButton::clicked, this, &MainWindow::selectScanPath);
    connect(cleanTempBtn, &QPushButton::clicked, this, &MainWindow::cleanTempFiles);
    connect(cleanCacheBtn, &QPushButton::clicked, this, &MainWindow::cleanCache);
    connect(customCleanBtn, &QPushButton::clicked, this, [this]() {
        // Select all checkboxes in the cleanup table
        int selectedCount = 0;
        for (int i = 0; i < cleanupTable->rowCount(); ++i) {
            QWidget *widget = cleanupTable->cellWidget(i, 3);
            if (widget) {
                QCheckBox *checkbox = widget->findChild<QCheckBox*>();
                if (checkbox) {
                    checkbox->setChecked(true);
                    selectedCount++;
                }
            }
        }
        if (selectedCount > 0) {
            addLog(QString("Selected %1 duplicate/old files for cleanup").arg(selectedCount), "INFO");
            QMessageBox::information(this, "Files Selected", 
                QString("Selected %1 files for cleanup.\nClick 'Perform Cleanup' to remove them.").arg(selectedCount));
        } else {
            QMessageBox::information(this, "No Files", 
                "No duplicate or old files found.\nScan a folder in the File Analyzer tab first.");
        }
    });
    connect(cleanupBtn, &QPushButton::clicked, this, &MainWindow::performCleanup);
    connect(browseSourceBtn, &QPushButton::clicked, this, &MainWindow::selectBackupSource);
    connect(browseDestBtn, &QPushButton::clicked, this, &MainWindow::selectBackupDestination);
    connect(createBackupBtn, &QPushButton::clicked, this, &MainWindow::createBackup);
    connect(monitorBtn, &QPushButton::clicked, this, &MainWindow::toggleMonitoring);

    QTimer *monitorTimer = new QTimer(this);
    connect(monitorTimer, &QTimer::timeout, this, [this]() {
        if (isMonitoring) {
            updateDiskInfo();
            updateMonitoringStats();
        }
    });
    monitorTimer->start(5000);

    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 0) {
            updateDiskInfo();
            updateBackupTable();
        }
    });

    updateBackupTable();
}

MainWindow::~MainWindow() {
    if (scanWorker && scanWorker->isRunning()) {
        scanWorker->terminate();
        scanWorker->wait();
    }
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QLabel *titleLabel = new QLabel("Spacemate - Disk Space Manager");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #2563eb; padding: 10px;");
    mainLayout->addWidget(titleLabel);

    tabWidget = new QTabWidget();
    mainLayout->addWidget(tabWidget);

    createDashboardTab();
    createAnalyzerTab();
    createCleanupTab();
    createBackupTab();
    createMonitorTab();

    QGroupBox *logGroup = new QGroupBox("Activity Log");
    QVBoxLayout *logLayout = new QVBoxLayout();

    logDisplay = new QTextEdit();
    logDisplay->setReadOnly(true);
    logDisplay->setMaximumHeight(150);
    logDisplay->setStyleSheet("background-color: #1e1e1e; color: #00ff00; font-family: monospace;");

    QPushButton *clearLogBtn = new QPushButton("Clear Log");
    connect(clearLogBtn, &QPushButton::clicked, this, &MainWindow::clearLogs);

    logLayout->addWidget(logDisplay);
    logLayout->addWidget(clearLogBtn);
    logGroup->setLayout(logLayout);
    mainLayout->addWidget(logGroup);

    setCentralWidget(centralWidget);
    addLog("Spacemate initialized successfully", "SUCCESS");
}

void MainWindow::createDashboardTab() {
    QWidget *dashboardWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(dashboardWidget);

    QGroupBox *storageGroup = new QGroupBox("Storage Overview");
    QVBoxLayout *storageLayout = new QVBoxLayout();

    QHBoxLayout *infoLayout = new QHBoxLayout();
    totalSpaceLabel = new QLabel("Total: 0 GB");
    usedSpaceLabel = new QLabel("Used: 0 GB");
    freeSpaceLabel = new QLabel("Free: 0 GB");

    totalSpaceLabel->setStyleSheet("padding: 10px; background: #dbeafe; border-radius: 5px; font-weight: bold;");
    usedSpaceLabel->setStyleSheet("padding: 10px; background: #fee2e2; border-radius: 5px; font-weight: bold;");
    freeSpaceLabel->setStyleSheet("padding: 10px; background: #dcfce7; border-radius: 5px; font-weight: bold;");

    infoLayout->addWidget(totalSpaceLabel);
    infoLayout->addWidget(usedSpaceLabel);
    infoLayout->addWidget(freeSpaceLabel);

    // Create segmented storage bar instead of progress bar
    QWidget *segmentedBarContainer = new QWidget();
    segmentedBarContainer->setObjectName("segmentedBarContainer");
    segmentedBarContainer->setMinimumHeight(60);
    segmentedBarContainer->setMaximumHeight(80);
    
    // This will be populated dynamically in updateDiskInfo()
    QHBoxLayout *segmentedLayout = new QHBoxLayout(segmentedBarContainer);
    segmentedLayout->setContentsMargins(0, 0, 0, 0);
    segmentedLayout->setSpacing(0);
    segmentedBarContainer->setLayout(segmentedLayout);

    usagePercentLabel = new QLabel("0%");
    usagePercentLabel->setAlignment(Qt::AlignCenter);
    QFont percentFont = usagePercentLabel->font();
    percentFont.setPointSize(14);
    percentFont.setBold(true);
    usagePercentLabel->setFont(percentFont);

    storageLayout->addLayout(infoLayout);
    storageLayout->addWidget(segmentedBarContainer);
    storageLayout->addWidget(usagePercentLabel);
    
    // Add detailed disk information label below progress bar
    QLabel *diskInfoLabel = new QLabel();
    diskInfoLabel->setObjectName("diskInfoLabel");
    diskInfoLabel->setStyleSheet("padding: 10px; color: #059669; font-family: monospace;");
    diskInfoLabel->setWordWrap(true);
    storageLayout->addWidget(diskInfoLabel);
    
    storageGroup->setLayout(storageLayout);

    QGroupBox *actionsGroup = new QGroupBox("Quick Actions");
    QGridLayout *actionsLayout = new QGridLayout();

    QPushButton *refreshBtn = new QPushButton("Refresh Dashboard");
    QPushButton *analyzeBtn = new QPushButton("Analyze Disk");
    QPushButton *cleanupBtn = new QPushButton("Quick Cleanup");
    QPushButton *backupBtn = new QPushButton("Create Backup");

    refreshBtn->setMinimumHeight(40);
    analyzeBtn->setMinimumHeight(40);
    cleanupBtn->setMinimumHeight(40);
    backupBtn->setMinimumHeight(40);

    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshDashboard);
    connect(analyzeBtn, &QPushButton::clicked, [this]() { tabWidget->setCurrentIndex(1); });
    connect(cleanupBtn, &QPushButton::clicked, [this]() { tabWidget->setCurrentIndex(2); });
    connect(backupBtn, &QPushButton::clicked, [this]() { tabWidget->setCurrentIndex(3); });

    actionsLayout->addWidget(refreshBtn, 0, 0);
    actionsLayout->addWidget(analyzeBtn, 0, 1);
    actionsLayout->addWidget(cleanupBtn, 1, 0);
    actionsLayout->addWidget(backupBtn, 1, 1);
    actionsGroup->setLayout(actionsLayout);

    layout->addWidget(storageGroup);
    layout->addWidget(actionsGroup);
    layout->addStretch();

    tabWidget->addTab(dashboardWidget, "Dashboard");
}

void MainWindow::createAnalyzerTab() {
    QWidget *analyzerWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(analyzerWidget);

    QGroupBox *scanGroup = new QGroupBox("Scan Directory");
    QHBoxLayout *scanLayout = new QHBoxLayout();

    scanPathInput = new QLineEdit();
    scanPathInput->setPlaceholderText("Enter path to scan...");

    browseScanBtn = new QPushButton("Browse");
    startScanBtn = new QPushButton("Start Scan");
    startScanBtn->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold;");

    scanLayout->addWidget(new QLabel("Path:"));
    scanLayout->addWidget(scanPathInput);
    scanLayout->addWidget(browseScanBtn);
    scanLayout->addWidget(startScanBtn);
    scanGroup->setLayout(scanLayout);

    scanProgressBar = new QProgressBar();
    scanProgressBar->setVisible(false);
    scanStatusLabel = new QLabel("Ready to scan");
    scanStatusLabel->setStyleSheet("padding: 5px; color: #059669;");

    fileTable = new QTableWidget();
    fileTable->setColumnCount(4);
    fileTable->setHorizontalHeaderLabels({"Path", "Size", "Type", "Action"});
    fileTable->horizontalHeader()->setStretchLastSection(true);
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(scanGroup);
    layout->addWidget(scanProgressBar);
    layout->addWidget(scanStatusLabel);
    layout->addWidget(fileTable);

    tabWidget->addTab(analyzerWidget, "File Analyzer");
}

void MainWindow::createCleanupTab() {
    QWidget *cleanupWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(cleanupWidget);

    QGroupBox *quickGroup = new QGroupBox("Quick Cleanup");
    QHBoxLayout *quickLayout = new QHBoxLayout();

    cleanTempBtn = new QPushButton("Clean Temp Files");
    cleanCacheBtn = new QPushButton("Clean Cache");
    customCleanBtn = new QPushButton("Select All Duplicates");
    
    // Add tooltips to explain what each button does
    cleanTempBtn->setToolTip("Clean system temporary files from /tmp and similar directories");
    cleanCacheBtn->setToolTip("Clean application cache files");
    customCleanBtn->setToolTip("Automatically select all duplicate files for cleanup");

    quickLayout->addWidget(cleanTempBtn);
    quickLayout->addWidget(cleanCacheBtn);
    quickLayout->addWidget(customCleanBtn);
    quickGroup->setLayout(quickLayout);

    cleanupTable = new QTableWidget();
    cleanupTable->setColumnCount(4);
    cleanupTable->setHorizontalHeaderLabels({"Path", "Size", "Type", "Select"});
    cleanupTable->horizontalHeader()->setStretchLastSection(true);
    cleanupStatusLabel = new QLabel("💡 Scan files in File Analyzer tab to find duplicates and old files");
    cleanupStatusLabel->setStyleSheet("padding: 10px; color: #2563eb; font-size: 13px;");

    cleanupBtn = new QPushButton("Perform Cleanup");
    cleanupBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: bold; padding: 10px;");

    layout->addWidget(quickGroup);
    layout->addWidget(cleanupTable);
    layout->addWidget(cleanupStatusLabel);
    layout->addWidget(cleanupBtn);

    tabWidget->addTab(cleanupWidget, "Cleanup");
}

void MainWindow::createBackupTab() {
    QWidget *backupWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(backupWidget);

    QGroupBox *createGroup = new QGroupBox("Create Backup");
    QVBoxLayout *createLayout = new QVBoxLayout();

    QHBoxLayout *sourceLayout = new QHBoxLayout();
    backupSourceInput = new QLineEdit();
    backupSourceInput->setPlaceholderText("Select source directory...");
    browseSourceBtn = new QPushButton("Browse");
    sourceLayout->addWidget(new QLabel("Source:"));
    sourceLayout->addWidget(backupSourceInput);
    sourceLayout->addWidget(browseSourceBtn);

    QHBoxLayout *destLayout = new QHBoxLayout();
    backupDestInput = new QLineEdit();
    backupDestInput->setPlaceholderText("Select destination directory...");
    browseDestBtn = new QPushButton("Browse");
    destLayout->addWidget(new QLabel("Destination:"));
    destLayout->addWidget(backupDestInput);
    destLayout->addWidget(browseDestBtn);

    createBackupBtn = new QPushButton("Create Backup");
    createBackupBtn->setStyleSheet("background-color: #7c3aed; color: white; font-weight: bold;");

    createLayout->addLayout(sourceLayout);
    createLayout->addLayout(destLayout);
    createLayout->addWidget(createBackupBtn);
    createGroup->setLayout(createLayout);

    QGroupBox *listGroup = new QGroupBox("Existing Backups");
    QVBoxLayout *listLayout = new QVBoxLayout();
    backupTable = new QTableWidget();
    backupTable->setColumnCount(5);
    backupTable->setHorizontalHeaderLabels({"Select", "Source", "Destination", "Date", "Action"});
    backupTable->horizontalHeader()->setStretchLastSection(true);
    
    // Add buttons for backup management
    QHBoxLayout *backupButtonsLayout = new QHBoxLayout();
    QPushButton *selectAllBackupsBtn = new QPushButton("Select All");
    QPushButton *deleteSelectedBackupsBtn = new QPushButton("Delete Selected");
    
    selectAllBackupsBtn->setStyleSheet("background-color: #3b82f6; color: white; font-weight: bold;");
    deleteSelectedBackupsBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: bold;");
    
    connect(selectAllBackupsBtn, &QPushButton::clicked, this, &MainWindow::selectAllBackups);
    connect(deleteSelectedBackupsBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedBackups);
    
    backupButtonsLayout->addWidget(selectAllBackupsBtn);
    backupButtonsLayout->addWidget(deleteSelectedBackupsBtn);
    backupButtonsLayout->addStretch();
    
    listLayout->addWidget(backupTable);
    listLayout->addLayout(backupButtonsLayout);
    listGroup->setLayout(listLayout);

    layout->addWidget(createGroup);
    layout->addWidget(listGroup);
    tabWidget->addTab(backupWidget, "Backup");
}

void MainWindow::createMonitorTab() {
    QWidget *monitorWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(monitorWidget);

    monitorStatusLabel = new QLabel("Monitoring stopped");
    monitorStatusLabel->setStyleSheet("color: #facc15; font-weight: bold; font-family: monospace;");
    monitorStatusLabel->setWordWrap(true);

    monitorBtn = new QPushButton("Start Monitoring");
    monitorBtn->setMinimumHeight(40);

    layout->addWidget(monitorStatusLabel);
    layout->addWidget(monitorBtn);
    layout->addStretch();

    tabWidget->addTab(monitorWidget, "Monitor");
}

void MainWindow::addLog(const QString &message, const QString &status) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logDisplay->append(QString("[%1] [%2] %3").arg(timestamp, status, message));
}

void MainWindow::clearLogs() {
    logDisplay->clear();
}

// DIAGNOSTIC VERSION - Replace your updateDiskInfo() with this temporarily
// This will help us see what's actually available on your system

void MainWindow::updateDiskInfo() {
    addLog("=== DIAGNOSTIC updateDiskInfo() ===", "INFO");
    
    // First, list ALL mounted volumes with full details
    addLog("========================================", "INFO");
    addLog("LISTING ALL MOUNTED VOLUMES:", "INFO");
    addLog("========================================", "INFO");
    
    int volNum = 0;
    for (const QStorageInfo& vol : QStorageInfo::mountedVolumes()) {
        volNum++;
        
        QString details = QString("\n📀 VOLUME %1:").arg(volNum);
        details += QString("\n  Root Path: %1").arg(vol.rootPath());
        details += QString("\n  Device: %1").arg(QString(vol.device()));
        details += QString("\n  File System: %1").arg(QString(vol.fileSystemType()));
        details += QString("\n  Name: %1").arg(vol.name());
        details += QString("\n  Display Name: %1").arg(vol.displayName());
        details += QString("\n  Is Valid: %1").arg(vol.isValid() ? "YES" : "NO");
        details += QString("\n  Is Ready: %1").arg(vol.isReady() ? "YES" : "NO");
        details += QString("\n  Is Read Only: %1").arg(vol.isReadOnly() ? "YES" : "NO");
        details += QString("\n  Total Bytes: %1").arg(vol.bytesTotal());
        details += QString("\n  Available Bytes: %1").arg(vol.bytesAvailable());
        
        if (vol.bytesTotal() > 0) {
            double totalGB = vol.bytesTotal() / (1024.0 * 1024.0 * 1024.0);
            double availGB = vol.bytesAvailable() / (1024.0 * 1024.0 * 1024.0);
            details += QString("\n  Total GB: %1").arg(totalGB, 0, 'f', 2);
            details += QString("\n  Available GB: %1").arg(availGB, 0, 'f', 2);
        }
        
        addLog(details, "INFO");
        addLog("----------------------------------------", "INFO");
    }
    
    if (volNum == 0) {
        addLog("❌ ERROR: QStorageInfo::mountedVolumes() returned NO volumes!", "ERROR");
    }
    
    // Now test specific paths
    addLog("\n========================================", "INFO");
    addLog("TESTING SPECIFIC PATHS:", "INFO");
    addLog("========================================", "INFO");
    
    QStringList testPaths = {
        "/mnt/c",          // Windows C: drive (primary)
        "/mnt/d",          // Windows D: drive if exists
        "/",               // WSL root (fallback)
        "/home",
        "/tmp",
        "/mnt/c/Users",
        QDir::homePath(),
        QDir::currentPath(),
        QDir::tempPath(),
    };
    
    QStorageInfo bestStorage;
    long long bestSize = 0;
    QString bestPath;
    
    for (const QString& path : testPaths) {
        QStorageInfo storage(path);
        
        QString testResult = QString("\n🔍 Testing: %1").arg(path);
        testResult += QString("\n  Exists: %1").arg(QDir(path).exists() ? "YES" : "NO");
        testResult += QString("\n  Valid: %1").arg(storage.isValid() ? "YES" : "NO");
        testResult += QString("\n  Ready: %1").arg(storage.isReady() ? "YES" : "NO");
        testResult += QString("\n  Root: %1").arg(storage.rootPath());
        testResult += QString("\n  Device: %1").arg(QString(storage.device()));
        testResult += QString("\n  Total Bytes: %1").arg(storage.bytesTotal());
        
        if (storage.bytesTotal() > 0) {
            double totalGB = storage.bytesTotal() / (1024.0 * 1024.0 * 1024.0);
            testResult += QString("\n  Total GB: %1").arg(totalGB, 0, 'f', 2);
            
            // Track the best one - prioritize Windows drives and reasonable sizes
            
            bool isWindowsDrive = path.startsWith("/mnt/c") || path.startsWith("/mnt/d");
            bool isReasonableSize = totalGB > 10 && totalGB < 500; // Between 10GB and 500GB (filters out virtual filesystems)
            
            if (storage.isValid() && storage.isReady()) {
                // Selection priority: Windows drives first, then reasonable-sized drives
                bool shouldSelect = false;
                
                if (isWindowsDrive && totalGB > 10) {
                    shouldSelect = true; // Always prefer Windows drives
                } else if (bestSize == 0 && isReasonableSize) {
                    shouldSelect = true; // If no selection yet, accept reasonable sizes
                } else if (isWindowsDrive && !bestPath.startsWith("/mnt/")) {
                    shouldSelect = true; // Upgrade from non-Windows to Windows drive
                }
                
                if (shouldSelect) {
                    bestSize = storage.bytesTotal();
                    bestStorage = storage;
                    bestPath = path;
                    testResult += QString("\n  ⭐ SELECTED AS BEST (Windows drive: %1, Size: %2 GB)")
                                   .arg(isWindowsDrive ? "YES" : "NO")
                                   .arg(totalGB, 0, 'f', 2);
                } else {
                    testResult += QString("\n  ⏭️ SKIPPED (Windows drive: %1, Reasonable size: %2)")
                                   .arg(isWindowsDrive ? "YES" : "NO")
                                   .arg(isReasonableSize ? "YES" : "NO");
                }
            }
            testResult += " ✅ VALID";
        } else {
            testResult += " ❌ INVALID";
        }
        
        addLog(testResult, "INFO");
        addLog("----------------------------------------", "INFO");
    }
    
    // Use the best storage found
    addLog("\n========================================", "INFO");
    addLog("FINAL RESULT:", "INFO");
    addLog("========================================", "INFO");
    
    if (bestSize > 0) {
        addLog(QString("✅ SUCCESS: Using path: %1").arg(bestPath), "SUCCESS");
        addLog(QString("   Root: %1").arg(bestStorage.rootPath()), "SUCCESS");
        addLog(QString("   Device: %1").arg(QString(bestStorage.device())), "SUCCESS");
        
        long long totalBytes = bestStorage.bytesTotal();
        long long freeBytes = bestStorage.bytesAvailable();
        long long usedBytes = totalBytes - freeBytes;
        
        double totalGB = totalBytes / (1024.0 * 1024.0 * 1024.0);
        double usedGB = usedBytes / (1024.0 * 1024.0 * 1024.0);
        double freeGB = freeBytes / (1024.0 * 1024.0 * 1024.0);
        
        addLog(QString("💾 Total: %1 GB").arg(totalGB, 0, 'f', 2), "SUCCESS");
        addLog(QString("💾 Used: %1 GB").arg(usedGB, 0, 'f', 2), "SUCCESS");
        addLog(QString("💾 Free: %1 GB").arg(freeGB, 0, 'f', 2), "SUCCESS");
        
        // Update UI
        totalSpaceLabel->setText(QString("Total: %1 GB").arg(totalGB, 0, 'f', 2));
        usedSpaceLabel->setText(QString("Used: %1 GB").arg(usedGB, 0, 'f', 2));
        freeSpaceLabel->setText(QString("Free: %1 GB").arg(freeGB, 0, 'f', 2));
        
        int percentUsed = (totalGB > 0) ? static_cast<int>((usedGB * 100.0) / totalGB) : 0;
        usagePercentLabel->setText(QString("%1%").arg(percentUsed));
        
        // Create segmented storage bar
        QWidget *segmentedBarContainer = findChild<QWidget*>("segmentedBarContainer");
        addLog(QString("🔍 Searching for segmentedBarContainer: %1").arg(segmentedBarContainer ? "FOUND" : "NOT FOUND"), "INFO");
        
        if (segmentedBarContainer) {
            addLog("📊 Creating segmented storage visualization...", "INFO");
            // Clear existing segments
            QLayout *oldLayout = segmentedBarContainer->layout();
            if (oldLayout) {
                QLayoutItem *item;
                while ((item = oldLayout->takeAt(0)) != nullptr) {
                    delete item->widget();
                    delete item;
                }
            }
            
            QHBoxLayout *segmentedLayout = new QHBoxLayout();
            segmentedLayout->setContentsMargins(0, 0, 0, 0);
            segmentedLayout->setSpacing(0);
            
            // Calculate category sizes (simplified categorization)
            // In a real scenario, you'd scan the filesystem for actual file types
            double documentsGB = usedGB * 0.25;  // 25% of used space
            double imagesGB = usedGB * 0.35;     // 35% of used space
            double appsGB = usedGB * 0.20;       // 20% of used space
            double systemGB = usedGB * 0.20;     // 20% of used space
            
            // Create segments
            struct Segment {
                QString name;
                double sizeGB;
                QString color;
                QString textColor;
            };
            
            QList<Segment> segments = {
                {"Documents", documentsGB, "#3b82f6", "#ffffff"},  // Blue
                {"Images", imagesGB, "#10b981", "#ffffff"},        // Green
                {"Apps", appsGB, "#8b5cf6", "#ffffff"},            // Purple
                {"System", systemGB, "#f59e0b", "#ffffff"},        // Orange
                {"Free", freeGB, "#e5e7eb", "#374151"}             // Gray
            };
            
            for (const auto &segment : segments) {
                double percentage = (segment.sizeGB * 100.0) / totalGB;
                if (percentage < 0.1) continue; // Skip tiny segments
                
                QWidget *segmentWidget = new QWidget();
                segmentWidget->setStyleSheet(QString(
                    "background-color: %1; border-right: 1px solid #ffffff;"
                ).arg(segment.color));
                
                QVBoxLayout *segmentLayout = new QVBoxLayout(segmentWidget);
                segmentLayout->setContentsMargins(5, 5, 5, 5);
                
                QLabel *nameLabel = new QLabel(segment.name);
                nameLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px;").arg(segment.textColor));
                nameLabel->setAlignment(Qt::AlignCenter);
                
                QLabel *sizeLabel = new QLabel(QString("%1 GB").arg(segment.sizeGB, 0, 'f', 1));
                sizeLabel->setStyleSheet(QString("color: %1; font-size: 10px;").arg(segment.textColor));
                sizeLabel->setAlignment(Qt::AlignCenter);
                
                segmentLayout->addWidget(nameLabel);
                segmentLayout->addWidget(sizeLabel);
                segmentWidget->setLayout(segmentLayout);
                
                segmentedLayout->addWidget(segmentWidget, percentage);
            }
            
            delete segmentedBarContainer->layout();
            segmentedBarContainer->setLayout(segmentedLayout);
            addLog(QString("✅ Segmented bar created with %1 segments").arg(segments.size()), "SUCCESS");
        } else {
            addLog("❌ ERROR: segmentedBarContainer not found! Bar not created.", "ERROR");
        }
        
        // Update detailed disk info label
        QLabel *diskInfoLabel = findChild<QLabel*>("diskInfoLabel");
        if (diskInfoLabel) {
            QString diskDetails;
            diskDetails += QString("📍 Mount Point: %1\n").arg(bestStorage.rootPath());
            diskDetails += QString("💾 Device: %1\n").arg(QString(bestStorage.device()));
            diskDetails += QString("📂 File System: %1\n").arg(QString(bestStorage.fileSystemType()));
            diskDetails += QString("🕐 Last Updated: %1")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
            diskInfoLabel->setText(diskDetails);
        }
        
        addLog(QString("✅ Dashboard updated: %1% used").arg(percentUsed), "SUCCESS");
        
    } else {
        addLog("❌ CRITICAL FAILURE: No valid storage found anywhere!", "ERROR");
        addLog("This might be a Qt or system configuration issue.", "ERROR");
        
        totalSpaceLabel->setText("Total: No valid storage");
        usedSpaceLabel->setText("Used: Check logs");
        freeSpaceLabel->setText("Free: Check logs");
        usagePercentLabel->setText("Error");
    }
    
    addLog("========================================", "INFO");
}

void MainWindow::refreshDashboard() {
    updateDiskInfo();
    updateBackupTable();
    addLog("Dashboard refreshed", "INFO");
}

void MainWindow::selectScanPath() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
    if (!path.isEmpty()) {
        path.replace("\\", "/");
        if (path.contains(":")) {
            QString drive = path.left(1).toLower();
            path = "/mnt/" + drive + path.mid(2);
        }
        scanPathInput->setText(path);
    }
}

void MainWindow::startScan() {
    if (isScanning) {
        QMessageBox::warning(this, "Scan in Progress", "A scan is already running!");
        return;
    }

    QString path = scanPathInput->text();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Invalid Path", "Please enter a valid path to scan.");
        return;
    }

    scanProgressBar->setVisible(true);
    scanProgressBar->setValue(0);
    scanStatusLabel->setText("Scanning...");
    fileTable->setRowCount(0);

    if (scanWorker) {
        scanWorker->deleteLater();
    }

    scanWorker = new ScanWorker(path.toStdString(), this);
    connect(scanWorker, &ScanWorker::scanProgress, scanProgressBar, &QProgressBar::setValue);
    connect(scanWorker, &ScanWorker::scanComplete, this, &MainWindow::onScanComplete);
    connect(scanWorker, &ScanWorker::scanError, this, &MainWindow::onScanError);

    isScanning = true;
    lastScannedPath = path;  // Track the scanned path for monitoring
    scanWorker->start();
    addLog(QString("Started scanning %1").arg(path), "INFO");
}

void MainWindow::populateCleanupTable(const ScanResults &results) {
    cleanupTable->setRowCount(0);
    int row = 0;
    long long totalCleanupSize = 0;
    
    for (const auto& file : results) {
        if (file.isDuplicate || file.isOld) {
            cleanupTable->insertRow(row);
            cleanupTable->setItem(row, 0, new QTableWidgetItem(file.path));
            
            QString sizeStr;
            if (file.size < 1024) {
                sizeStr = QString("%1 B").arg(file.size);
            } else if (file.size < 1024 * 1024) {
                sizeStr = QString("%1 KB").arg(file.size / 1024.0, 0, 'f', 2);
            } else if (file.size < 1024 * 1024 * 1024) {
                sizeStr = QString("%1 MB").arg(file.size / (1024.0 * 1024.0), 0, 'f', 2);
            } else {
                sizeStr = QString("%1 GB").arg(file.size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
            }
            cleanupTable->setItem(row, 1, new QTableWidgetItem(sizeStr));
            
            QString type = file.isDuplicate ? "Duplicate" : "Old File";
            if (file.isDuplicate && file.isOld) {
                type = "Duplicate & Old";
            }
            QTableWidgetItem* typeItem = new QTableWidgetItem(type);
            typeItem->setBackground(QColor(255, 200, 200));
            cleanupTable->setItem(row, 2, typeItem);
            
            QWidget *checkWidget = new QWidget();
            QCheckBox *checkbox = new QCheckBox();
            checkbox->setChecked(false);
            QHBoxLayout *layout = new QHBoxLayout(checkWidget);
            layout->addWidget(checkbox);
            layout->setAlignment(Qt::AlignCenter);
            layout->setContentsMargins(0, 0, 0, 0);
            cleanupTable->setCellWidget(row, 3, checkWidget);
            
            totalCleanupSize += file.size;
            row++;
        }
    }
    
    QString sizeStr;
    if (totalCleanupSize < 1024 * 1024) {
        sizeStr = QString("%1 KB").arg(totalCleanupSize / 1024.0, 0, 'f', 2);
    } else if (totalCleanupSize < 1024 * 1024 * 1024) {
        sizeStr = QString("%1 MB").arg(totalCleanupSize / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        sizeStr = QString("%1 GB").arg(totalCleanupSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    
    if (row > 0) {
        cleanupStatusLabel->setText(QString("Found %1 files ready for cleanup (%2)")
                                    .arg(row).arg(sizeStr));
        cleanupStatusLabel->setStyleSheet("padding: 5px; color: #dc2626; font-weight: bold;");
    } else {
        cleanupStatusLabel->setText("No files marked for cleanup. All files are clean!");
        cleanupStatusLabel->setStyleSheet("padding: 5px; color: #059669;");
    }
    
    cleanupTable->resizeColumnsToContents();
}

void MainWindow::onScanComplete(const ScanResults &results, const DuplicateGroups &duplicates) {
    fileTable->setRowCount(results.size());
    
    long long totalSize = 0;
    int fileCount = results.size();
    int duplicateCount = 0;
    int oldFileCount = 0;
    
    fileTable->setColumnCount(6);
    fileTable->setHorizontalHeaderLabels({"Path", "Size", "Type", "Last Modified", "Status", "Action"});

    for (size_t i = 0; i < results.size(); ++i) {
        fileTable->setItem(i, 0, new QTableWidgetItem(results[i].path));
        
        long long sizeBytes = results[i].size;
        totalSize += sizeBytes;
        QString sizeStr;
        if (sizeBytes < 1024) {
            sizeStr = QString("%1 B").arg(sizeBytes);
        } else if (sizeBytes < 1024 * 1024) {
            sizeStr = QString("%1 KB").arg(sizeBytes / 1024.0, 0, 'f', 2);
        } else if (sizeBytes < 1024 * 1024 * 1024) {
            sizeStr = QString("%1 MB").arg(sizeBytes / (1024.0 * 1024.0), 0, 'f', 2);
        } else {
            sizeStr = QString("%1 GB").arg(sizeBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        }
        fileTable->setItem(i, 1, new QTableWidgetItem(sizeStr));
        
        fileTable->setItem(i, 2, new QTableWidgetItem(results[i].type.isEmpty() ? "File" : results[i].type));
        fileTable->setItem(i, 3, new QTableWidgetItem(results[i].lastModified));

        QString status;
        if (results[i].isDuplicate) {
            status = "Duplicate";
            duplicateCount++;
        }
        if (results[i].isOld) {
            status += status.isEmpty() ? "Old" : ", Old";
            oldFileCount++;
        }
        if (status.isEmpty()) status = "Normal";
        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        if (results[i].isDuplicate || results[i].isOld) {
            statusItem->setBackground(QColor(255, 200, 200));
        }
        fileTable->setItem(i, 4, statusItem);
        
        QPushButton *actionBtn = new QPushButton("Delete");
        if (results[i].isDuplicate || results[i].isOld) {
            actionBtn->setStyleSheet("background-color: #dc2626; color: white;");
        } else {
            actionBtn->setStyleSheet("background-color: #666666; color: white;");
        }
        fileTable->setCellWidget(i, 5, actionBtn);
        
        connect(actionBtn, &QPushButton::clicked, [this, detail=results[i]]() { 
            deleteFileFromPath(detail.path);
        });
    }
    
    fileTable->resizeColumnToContents(1);
    fileTable->resizeColumnToContents(2);
    fileTable->resizeColumnToContents(3);
    
    QString totalSizeStr;
    if (totalSize < 1024 * 1024 * 1024) {
        totalSizeStr = QString("%1 MB").arg(totalSize / (1024.0 * 1024.0), 0, 'f', 2);
    } else {
        totalSizeStr = QString("%1 GB").arg(totalSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }
    
    QString summary = QString("Scan complete:\n");
    summary += QString("- Total files: %1\n").arg(fileCount);
    summary += QString("- Total size: %1\n").arg(totalSizeStr);
    if (duplicateCount > 0) {
        summary += QString("- Duplicate files: %1\n").arg(duplicateCount);
    }
    if (oldFileCount > 0) {
        summary += QString("- Old files (>90 days): %1\n").arg(oldFileCount);
    }

    scanStatusLabel->setText(summary);
    scanProgressBar->setVisible(false);
    isScanning = false;
    
    // Populate cleanup table with duplicates and old files
    populateCleanupTable(results);
    
    // Log detailed folder statistics
    QString folderPath = scanPathInput->text();
    addLog("═══════════════════════════════════════", "INFO");
    addLog("📊 SCAN COMPLETE - DETAILED STATISTICS", "SUCCESS");
    addLog("═══════════════════════════════════════", "INFO");
    addLog(QString("📂 Scanned Folder: %1").arg(folderPath), "INFO");
    addLog(QString("📦 Folder Size: %1").arg(totalSizeStr), "INFO");
    addLog(QString("📄 Total Files: %1").arg(fileCount), "INFO");
    
    if (duplicateCount > 0) {
        addLog(QString("🔄 Duplicate Files Found: %1").arg(duplicateCount), "INFO");
    }
    if (oldFileCount > 0) {
        addLog(QString("⏰ Old Files (>90 days): %1").arg(oldFileCount), "INFO");
    }
    
    // Count temporary files
    int tempCount = 0;
    for (const auto& file : results) {
        if (file.path.contains("/tmp/") || file.path.contains("\\temp\\", Qt::CaseInsensitive) ||
            file.path.endsWith(".tmp") || file.path.endsWith(".cache")) {
            tempCount++;
        }
    }
    if (tempCount > 0) {
        addLog(QString("🗑️  Temporary Files: %1").arg(tempCount), "INFO");
    }
    
    addLog("═══════════════════════════════════════", "INFO");
    
    // Auto-delete files older than 45 days
    QDateTime autoDeleteThreshold = QDateTime::currentDateTime().addDays(-45);
    int autoDeletedCount = 0;
    long long autoDeletedSize = 0;
    
    for (const auto& file : results) {
        QDateTime fileDate = QDateTime::fromString(file.lastModified, "yyyy-MM-dd HH:mm:ss");
        if (fileDate.isValid() && fileDate < autoDeleteThreshold) {
            try {
                if (fs::remove(file.path.toStdString())) {
                    autoDeletedCount++;
                    autoDeletedSize += file.size;
                    addLog(QString("🗑️  Auto-deleted old file: %1").arg(file.path), "SUCCESS");
                }
            } catch (const std::exception& e) {
                addLog(QString("❌ Failed to auto-delete: %1 - %2").arg(file.path, e.what()), "ERROR");
            }
        }
    }
    
    if (autoDeletedCount > 0) {
        QString deletedSizeStr;
        if (autoDeletedSize < 1024 * 1024 * 1024) {
            deletedSizeStr = QString("%1 MB").arg(autoDeletedSize / (1024.0 * 1024.0), 0, 'f', 2);
        } else {
            deletedSizeStr = QString("%1 GB").arg(autoDeletedSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        }
        addLog("═══════════════════════════════════════", "SUCCESS");
        addLog(QString("🧹 AUTO-CLEANUP: Deleted %1 files older than 45 days").arg(autoDeletedCount), "SUCCESS");
        addLog(QString("💾 Freed Space: %1").arg(deletedSizeStr), "SUCCESS");
        addLog("═══════════════════════════════════════", "SUCCESS");
        
        QMessageBox::information(this, "Auto-Cleanup Complete",
            QString("Automatically deleted %1 files older than 45 days.\nFreed %2 of space.")
            .arg(autoDeletedCount).arg(deletedSizeStr));
        
        // Refresh the display after auto-deletion
        updateDiskInfo();
    }
    
    if (duplicateCount > 0 || oldFileCount > 0) {
        QMessageBox::information(this, "Cleanup Suggestion",
            QString("Found %1 duplicate files and %2 old files.\n"
                   "Go to the Cleanup tab to review and remove them.")
                   .arg(duplicateCount).arg(oldFileCount));
    }

    addLog(QString("Scan completed: %1 files (%2 duplicates, %3 old) found, %4 total size")
           .arg(fileCount).arg(duplicateCount).arg(oldFileCount).arg(totalSizeStr), "SUCCESS");
}

void MainWindow::onScanError(const QString &error) {
    QMessageBox::critical(this, "Scan Error", error);
    scanStatusLabel->setText("Scan failed");
    scanProgressBar->setVisible(false);
    isScanning = false;
    addLog(QString("Scan failed: %1").arg(error), "ERROR");
}

void MainWindow::deleteFileFromTable(int row) {
    QTableWidgetItem *item = fileTable->item(row, 0);
    if (!item) return;
    QString path = item->text();
    deleteFileFromPath(path);
}

void MainWindow::deleteFileFromPath(const QString &path) {
    if (QMessageBox::question(this, "Delete File", 
        QString("Are you sure you want to delete:\n%1?").arg(path),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        
        try {
            if (fs::remove(path.toStdString())) {
                addLog(QString("Deleted file: %1").arg(path), "SUCCESS");
                
                for (int i = 0; i < fileTable->rowCount(); ++i) {
                    QTableWidgetItem *item = fileTable->item(i, 0);
                    if (item && item->text() == path) {
                        fileTable->removeRow(i);
                        break;
                    }
                }
                
                updateDiskInfo();
            } else {
                addLog(QString("Failed to delete file: %1").arg(path), "ERROR");
                QMessageBox::warning(this, "Delete Failed", "Could not delete the file.");
            }
        } catch (const std::exception &e) {
            addLog(QString("Error deleting file: %1 - %2").arg(path, e.what()), "ERROR");
            QMessageBox::critical(this, "Error", QString("Error: %1").arg(e.what()));
        }
    }
}

void MainWindow::cleanTempFiles() {
    QMessageBox::StandardButton confirm = QMessageBox::question(this,
        "Clean Temporary Files",
        "This will clean temporary files from system temp directories.\nDo you want to continue?",
        QMessageBox::Yes | QMessageBox::No);
        
    if (confirm == QMessageBox::Yes) {
        QProgressDialog progress("Cleaning temporary files...", "Cancel", 0, 100, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        progress.setValue(0);
        
        QFuture<void> future = QtConcurrent::run([this]() {
            cleanupManager->cleanTempFiles();
        });
        
        while (!future.isFinished()) {
            QCoreApplication::processEvents();
            progress.setValue(progress.value() + 1);
            if (progress.value() >= 100) progress.setValue(0);
            QThread::msleep(100);
        }
        
        progress.setValue(100);
        updateDiskInfo();
        addLog("Temporary files cleaned successfully", "SUCCESS");
        QMessageBox::information(this, "Cleanup Complete", "Temporary files have been cleaned successfully.");
    }
}

void MainWindow::cleanCache() {
    QMessageBox::StandardButton confirm = QMessageBox::question(this,
        "Clean Cache Files",
        "This will clean cache files from system cache directories.\nDo you want to continue?",
        QMessageBox::Yes | QMessageBox::No);
        
    if (confirm == QMessageBox::Yes) {
        QProgressDialog progress("Cleaning cache files...", "Cancel", 0, 100, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.setMinimumDuration(0);
        progress.setValue(0);
        
        QFuture<void> future = QtConcurrent::run([this]() {
            cleanupManager->cleanCache();
        });
        
        while (!future.isFinished()) {
            QCoreApplication::processEvents();
            progress.setValue(progress.value() + 1);
            if (progress.value() >= 100) progress.setValue(0);
            QThread::msleep(100);
        }
        
        progress.setValue(100);
        updateDiskInfo();
        addLog("Cache files cleaned successfully", "SUCCESS");
        QMessageBox::information(this, "Cleanup Complete", "Cache files have been cleaned successfully.");
    }
}

void MainWindow::performCleanup() {
    // Collect selected paths first (so we don't mutate table while iterating forward)
    QVector<QString> selectedPaths;
    QVector<QString> backupPaths;

    for (int i = 0; i < cleanupTable->rowCount(); ++i) {
        QWidget *widget = cleanupTable->cellWidget(i, 3);
        QCheckBox *checkbox = widget ? widget->findChild<QCheckBox*>() : nullptr;
        if (checkbox && checkbox->isChecked()) {
            QString path = cleanupTable->item(i, 0)->text();
            selectedPaths.append(path);

            // Create backup path under backupManager path with timestamp (HH for 24-hour)
            QString backupRoot = QString::fromStdString(backupManager->getBackupDir());
            QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
            QString backupPath = backupRoot + "/" + stamp + "_" + QFileInfo(path).fileName();
            backupPaths.append(backupPath);
        }
    }

    if (selectedPaths.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select items to clean up.");
        return;
    }

    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, "Confirm Cleanup",
        QString("Would you like to backup the selected files before cleaning?\nSelected items: %1").arg(selectedPaths.size()),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (confirm == QMessageBox::Cancel) return;

    // If backup requested, ensure backupRoot exists
    if (confirm == QMessageBox::Yes) {
        QString backupRoot = QString::fromStdString(backupManager->getBackupDir());
        if (backupRoot.isEmpty() || !QDir(backupRoot).exists()) {
            QMessageBox::warning(this, "Invalid Backup Path", "Backup path not set or invalid. Please check Backup tab.");
            return;
        }
    }

    cleanupBtn->setEnabled(false);
    cleanTempBtn->setEnabled(false);
    cleanCacheBtn->setEnabled(false);

    QProgressDialog progress("Performing cleanup...", "Cancel", 0,
                             selectedPaths.size() * (confirm == QMessageBox::Yes ? 2 : 1), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    int step = 0;
    int cleaned = 0;

    for (int i = 0; i < selectedPaths.size(); ++i) {
        const QString &path = selectedPaths[i];
        progress.setValue(step++);
        if (progress.wasCanceled()) break;

        // Backup if requested
        if (confirm == QMessageBox::Yes) {
            // Ensure destination folder exists
            QString destPath = QFileInfo(backupPaths[i]).absolutePath();
            QDir().mkpath(destPath);
            if (QFile::copy(path, backupPaths[i])) {
                addLog(QString("Backed up: %1 -> %2").arg(path, backupPaths[i]), "SUCCESS");
                // Add to backup index so it shows in Existing Backups
                backupManager->addBackupIndexEntry(path.toStdString(), backupPaths[i].toStdString());
            } else {
                addLog(QString("Failed to backup: %1").arg(path), "ERROR");
                // don't delete if backup fails; continue to next file
                continue;
            }
            progress.setValue(step++);
        }

        // Delete file
        try {
            if (QFile::remove(path)) {
                addLog(QString("Cleaned: %1").arg(path), "SUCCESS");
                cleaned++;
                // Also remove any matching row from cleanupTable (search and remove)
                for (int r = 0; r < cleanupTable->rowCount(); ++r) {
                    QTableWidgetItem *it = cleanupTable->item(r, 0);
                    if (it && it->text() == path) {
                        cleanupTable->removeRow(r);
                        break;
                    }
                }
            } else {
                addLog(QString("Failed to clean: %1").arg(path), "ERROR");
            }
        } catch (const std::exception &e) {
            addLog(QString("Error cleaning %1 - %2").arg(path, e.what()), "ERROR");
        }
    }

    progress.setValue(progress.maximum());
    updateDiskInfo();
    updateBackupTable();  // Refresh backup table to show new backups

    cleanupBtn->setEnabled(true);
    cleanTempBtn->setEnabled(true);
    cleanCacheBtn->setEnabled(true);

    cleanupStatusLabel->setText("Cleanup completed successfully.");
    QMessageBox::information(this, "Cleanup Complete",
                             QString("Cleaned %1 of %2 items successfully.").arg(cleaned).arg(selectedPaths.size()));
}

// ==================== Backup Slots ====================
QString MainWindow::convertToWSLPath(const QString &windowsPath) {
    QString path = windowsPath;
    path.replace("\\", "/");
    if (path.contains(":")) {
        QString drive = path.left(1).toLower();
        path = "/mnt/" + drive + path.mid(2);
    }
    return path;
}

void MainWindow::selectBackupSource() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Source Directory");
    if (!path.isEmpty()) {
        path = convertToWSLPath(path);
        backupSourceInput->setText(path);
    }
}

void MainWindow::selectBackupDestination() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Destination Directory");
    if (!path.isEmpty()) {
        path = convertToWSLPath(path);
        backupDestInput->setText(path);
    }
}

void MainWindow::updateBackupTable() {
    backupTable->setRowCount(0);
    auto backups = backupManager->loadBackupIndex();
    
    for (const auto& backup : backups) {
        int row = backupTable->rowCount();
        backupTable->insertRow(row);
        
        // Add checkbox
        QCheckBox *checkbox = new QCheckBox();
        backupTable->setCellWidget(row, 0, checkbox);
        
        backupTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(backup.originalPath)));
        backupTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(backup.backupPath)));
        backupTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(backup.timestamp)));
        
        QPushButton *restoreBtn = new QPushButton("Restore");
        restoreBtn->setStyleSheet("background-color: #7c3aed; color: white;");
        
        connect(restoreBtn, &QPushButton::clicked, [this, backup]() {
            QMessageBox::StandardButton confirm = QMessageBox::question(this,
                "Restore Backup",
                QString("Do you want to restore the backup from %1?\n\n"
                       "This will overwrite the current files at the original location.")
                       .arg(QString::fromStdString(backup.timestamp)),
                QMessageBox::Yes | QMessageBox::No);
                
            if (confirm == QMessageBox::Yes) {
                QProgressDialog progress("Restoring backup...", "Cancel", 0, 100, this);
                progress.setWindowModality(Qt::WindowModal);
                progress.setMinimumDuration(0);
                
                QFuture<bool> future = QtConcurrent::run([this, &backup]() {
                    try {
                        return fs::copy_file(backup.backupPath, backup.originalPath, 
                                          fs::copy_options::overwrite_existing);
                    } catch (...) {
                        return false;
                    }
                });
                
                while (!future.isFinished()) {
                    QCoreApplication::processEvents();
                    progress.setValue(progress.value() + 1);
                    if (progress.value() >= 100) progress.setValue(0);
                    QThread::msleep(100);
                }
                
                bool success = future.result();
                progress.setValue(100);
                
                if (success) {
                    addLog(QString("Backup restored: %1").arg(QString::fromStdString(backup.backupPath)), "SUCCESS");
                    QMessageBox::information(this, "Restore Complete", "Backup has been restored successfully.");
                } else {
                    addLog(QString("Failed to restore backup: %1").arg(QString::fromStdString(backup.backupPath)), "ERROR");
                    QMessageBox::critical(this, "Restore Failed", "Failed to restore the backup.");
                }
            }
        });
        
        QWidget *btnContainer = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(btnContainer);
        layout->addWidget(restoreBtn);
        layout->setContentsMargins(0, 0, 0, 0);
        backupTable->setCellWidget(row, 4, btnContainer);
    }
    
    backupTable->resizeColumnsToContents();
}

void MainWindow::createBackup() {
    QString src = backupSourceInput->text();
    QString dest = backupDestInput->text();
    
    if (src.isEmpty() || dest.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Both source and destination must be selected.");
        return;
    }

    if (!src.startsWith("/")) src = convertToWSLPath(src);
    if (!dest.startsWith("/")) dest = convertToWSLPath(dest);

    if (!QDir(src).exists()) {
        QMessageBox::warning(this, "Invalid Source", "Source directory does not exist.");
        return;
    }

    QDir().mkpath(dest);

    QProgressDialog progress("Creating backup...", "Cancel", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    progress.setValue(0);

    QFuture<std::string> future = QtConcurrent::run([this, src, dest]() {
        return backupManager->createBackup(src.toStdString(), dest.toStdString());
    });

    while (!future.isFinished()) {
        QCoreApplication::processEvents();
        progress.setValue(progress.value() + 1);
        if (progress.value() >= 100) progress.setValue(0);
        QThread::msleep(100);
    }

    std::string backupPath = future.result();
    progress.setValue(100);

    if (!backupPath.empty()) {
        updateBackupTable();
        addLog(QString("Backup created: %1").arg(QString::fromStdString(backupPath)), "SUCCESS");
        QMessageBox::information(this, "Backup Complete", 
            QString("Backup has been created successfully at:\n%1").arg(QString::fromStdString(backupPath)));
    } else {
        addLog(QString("Failed to create backup from %1 to %2").arg(src, dest), "ERROR");
        QMessageBox::critical(this, "Backup Failed", "Failed to create the backup. Check the logs for details.");
    }
}

void MainWindow::updateMonitoringStats() {
    if (!isMonitoring) {
        return;
    }

    addLog("=== updateMonitoringStats() called ===", "INFO");

    // Use the last scanned path if available, otherwise use root
    QString monitorPath = lastScannedPath.isEmpty() ? "/" : lastScannedPath;
    QStorageInfo storage(monitorPath);
    bool foundValid = storage.isValid() && storage.isReady() && storage.bytesTotal() > 0;
    
    // If the scanned path doesn't work, try fallback mount points
    if (!foundValid) {
        QStringList mountPoints = {
            QDir::homePath(),
            "/home",
            "/mnt/c",
            "/",
        };
        
        for (const QString& mp : mountPoints) {
            storage = QStorageInfo(mp);
            if (storage.isValid() && storage.isReady() && storage.bytesTotal() > 0) {
                foundValid = true;
                monitorPath = mp;
                addLog(QString("Monitoring using fallback: %1").arg(mp), "SUCCESS");
                break;
            }
        }
    } else {
        addLog(QString("Monitoring scanned path: %1").arg(monitorPath), "SUCCESS");
    }
    
    if (!foundValid) {
        monitorStatusLabel->setText("❌ MONITORING ERROR\n\n"
                                   "Cannot access disk information.\n"
                                   "No valid storage mount points available.\n\n"
                                   "Check Activity Log for details.");
        monitorStatusLabel->setStyleSheet("color: #dc2626; font-weight: bold; font-family: monospace;");
        addLog("❌ MONITORING FAILED: No valid storage", "ERROR");
        return;
    }

    // Calculate folder-specific statistics if monitoring a specific path
    long long folderSize = 0;
    int totalFiles = 0;
    int totalFolders = 0;
    int duplicateFiles = 0;
    int oldFiles = 0;
    int tempFiles = 0;
    
    bool isMonitoringFolder = !lastScannedPath.isEmpty() && lastScannedPath != "/";
    
    if (isMonitoringFolder && QDir(lastScannedPath).exists()) {
        try {
            QDateTime oldThreshold = QDateTime::currentDateTime().addDays(-45);
            std::unordered_map<QString, int> hashCounts;
            
            for (const auto &entry : fs::recursive_directory_iterator(
                     lastScannedPath.toStdString(), 
                     fs::directory_options::skip_permission_denied)) {
                
                if (entry.is_directory()) {
                    totalFolders++;
                } else if (entry.is_regular_file()) {
                    totalFiles++;
                    folderSize += entry.file_size();
                    
                    QString path = QString::fromStdString(entry.path().string());
                    
                    // Check if old file
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        entry.last_write_time() - fs::file_time_type::clock::now() + 
                        std::chrono::system_clock::now());
                    if (QDateTime::fromSecsSinceEpoch(
                        std::chrono::system_clock::to_time_t(sctp)) < oldThreshold) {
                        oldFiles++;
                    }
                    
                    // Check if temporary file
                    if (path.contains("/tmp/") || path.contains("\\temp\\", Qt::CaseInsensitive) ||
                        path.endsWith(".tmp") || path.endsWith(".cache")) {
                        tempFiles++;
                    }
                    
                    // Calculate hash for duplicate detection (only for files > 1KB)
                    if (entry.file_size() > 1024) {
                        QFile file(path);
                        if (file.open(QFile::ReadOnly)) {
                            QCryptographicHash hash(QCryptographicHash::Md5);
                            hash.addData(&file);
                            QString hashStr = QString(hash.result().toHex());
                            hashCounts[hashStr]++;
                            file.close();
                        }
                    }
                }
            }
            
            // Count duplicates
            for (const auto& pair : hashCounts) {
                if (pair.second > 1) {
                    duplicateFiles += pair.second - 1; // Count extras as duplicates
                }
            }
            
        } catch (const std::exception& e) {
            addLog(QString("Error calculating folder stats: %1").arg(e.what()), "ERROR");
        }
    }
    
    // Calculate storage statistics
    double totalGB = storage.bytesTotal() / (1024.0 * 1024.0 * 1024.0);
    double freeGB = storage.bytesAvailable() / (1024.0 * 1024.0 * 1024.0);
    double usedGB = totalGB - freeGB;
    double freePercent = (freeGB * 100.0) / totalGB;
    double usedPercent = 100.0 - freePercent;

    // Build monitoring display
    QString stats = "🟢 MONITORING ACTIVE\n\n";
    stats += "═══════════════════════════════\n";
    
    if (isMonitoringFolder) {
        stats += QString("📂 Monitored Folder:\n%1\n").arg(lastScannedPath);
        stats += "───────────────────────────────\n";
        
        // Folder size
        double folderSizeGB = folderSize / (1024.0 * 1024.0 * 1024.0);
        double folderSizeMB = folderSize / (1024.0 * 1024.0);
        if (folderSizeGB >= 0.1) {
            stats += QString("📊 Folder Size: %1 GB\n").arg(folderSizeGB, 0, 'f', 2);
        } else {
            stats += QString("📊 Folder Size: %1 MB\n").arg(folderSizeMB, 0, 'f', 2);
        }
        
        stats += QString("📄 Total Files: %1\n").arg(totalFiles);
        stats += QString("📁 Total Folders: %1\n").arg(totalFolders);
        stats += "───────────────────────────────\n";
        
        // File categories
        if (duplicateFiles > 0) {
            stats += QString("🔄 Duplicate Files: %1\n").arg(duplicateFiles);
        }
        if (tempFiles > 0) {
            stats += QString("🗑️  Temporary Files: %1\n").arg(tempFiles);
        }
        if (oldFiles > 0) {
            stats += QString("⏰ Old Files (>45 days): %1\n").arg(oldFiles);
        }
        
        if (duplicateFiles == 0 && tempFiles == 0 && oldFiles == 0) {
            stats += "✅ No duplicates or old files\n";
        }
        
        stats += "═══════════════════════════════\n\n";
    }
    
    // Disk information
    stats += "💾 DISK INFORMATION\n";
    stats += "───────────────────────────────\n";
    stats += QString("📍 Mount Point: %1\n").arg(storage.rootPath());
    stats += QString("� Device: %1\n").arg(QString(storage.device()));
    stats += QString("📊 Disk Usage: %1%\n").arg(usedPercent, 0, 'f', 1);
    stats += "───────────────────────────────\n";
    stats += QString("📦 Total Space: %1 GB\n").arg(totalGB, 0, 'f', 2);
    stats += QString("✅ Free Space: %1 GB (%2%)\n").arg(freeGB, 0, 'f', 2).arg(freePercent, 0, 'f', 1);
    stats += QString("📉 Used Space: %1 GB (%2%)\n").arg(usedGB, 0, 'f', 2).arg(usedPercent, 0, 'f', 1);
    stats += "═══════════════════════════════\n\n";

    // Try to get largest directories (with error handling)
    try {
        auto dirs = diskMonitor->getLargestDirectories(storage.rootPath().toStdString(), 5);
        if (!dirs.empty()) {
            stats += "📁 Largest Directories:\n";
            stats += "───────────────────────────────\n";
            int count = 0;
            for (const auto& dir : dirs) {
                if (count >= 5) break;
                QString dirPath = QString::fromStdString(dir.first);
                double sizeGB = dir.second / (1024.0 * 1024.0 * 1024.0);
                double sizeMB = dir.second / (1024.0 * 1024.0);
                
                if (sizeGB >= 0.1) {
                    stats += QString("  • %1: %2 GB\n").arg(dirPath).arg(sizeGB, 0, 'f', 2);
                    count++;
                } else if (sizeMB >= 1.0) {
                    stats += QString("  • %1: %2 MB\n").arg(dirPath).arg(sizeMB, 0, 'f', 1);
                    count++;
                }
            }
            if (count > 0) {
                stats += "\n";
            }
        }
    } catch (const std::exception& e) {
        addLog(QString("getLargestDirectories error: %1").arg(e.what()), "ERROR");
    } catch (...) {
        // Silently ignore if largest directories not available
    }

    // Add warnings based on disk space
    if (freePercent < 10) {
        stats += "⚠️  CRITICAL WARNING ⚠️\n";
        stats += "═══════════════════════════════\n";
        stats += "🔴 CRITICALLY LOW DISK SPACE!\n";
        stats += QString("Only %1 GB (%2%) remaining\n")
                 .arg(freeGB, 0, 'f', 2)
                 .arg(freePercent, 0, 'f', 1);
        stats += "Action required immediately!\n\n";
    } else if (freePercent < 20) {
        stats += "⚠️  CAUTION ⚠️\n";
        stats += "───────────────────────────────\n";
        stats += QString("🟡 Low disk space: %1 GB (%2%)\n")
                 .arg(freeGB, 0, 'f', 2)
                 .arg(freePercent, 0, 'f', 1);
        stats += "Consider cleaning up files\n\n";
    }
    
    stats += QString("⏰ Last updated: %1")
             .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));

    // Update the label
    monitorStatusLabel->setText(stats);
    
    // Set color based on disk usage
    QString color;
    if (freePercent < 10) {
        color = "#dc2626"; // Red
    } else if (freePercent < 20) {
        color = "#f59e0b"; // Orange
    } else {
        color = "#059669"; // Green
    }
    monitorStatusLabel->setStyleSheet(
        QString("color: %1; font-weight: bold; font-family: 'Courier New', monospace; padding: 10px;").arg(color)
    );
}

void MainWindow::toggleMonitoring() {
    addLog(QString("=== toggleMonitoring - Current state: %1 ===")
           .arg(isMonitoring ? "ON" : "OFF"), "INFO");
    
    if (isMonitoring) {
        // STOP monitoring
        addLog("🛑 Stopping monitoring...", "INFO");
        
        try {
            diskMonitor->stopMonitoring();
            addLog("diskMonitor->stopMonitoring() succeeded", "SUCCESS");
        } catch (const std::exception& e) {
            addLog(QString("diskMonitor->stopMonitoring() error: %1").arg(e.what()), "ERROR");
        } catch (...) {
            addLog("diskMonitor->stopMonitoring() unknown error", "ERROR");
        }
        
        monitorBtn->setText("Start Monitoring");
        monitorBtn->setStyleSheet("");
        monitorStatusLabel->setText("⏸️  MONITORING STOPPED\n\n"
                                   "Click 'Start Monitoring' to begin\n"
                                   "real-time disk monitoring.\n\n"
                                   "Updates will occur every 5 seconds.");
        monitorStatusLabel->setStyleSheet("color: #facc15; font-weight: bold; font-family: monospace; padding: 10px;");
        isMonitoring = false;
        addLog("✅ Disk monitoring stopped successfully", "SUCCESS");
        
    } else {
        // START monitoring
        addLog("▶️ Starting monitoring...", "INFO");
        
        try {
            diskMonitor->startMonitoring();
            addLog("diskMonitor->startMonitoring() succeeded", "SUCCESS");
        } catch (const std::exception& e) {
            addLog(QString("diskMonitor->startMonitoring() error: %1").arg(e.what()), "ERROR");
        } catch (...) {
            addLog("diskMonitor->startMonitoring() unknown error", "ERROR");
        }
        
        monitorBtn->setText("Stop Monitoring");
        monitorBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: bold;");
        isMonitoring = true;
        
        addLog("Calling updateMonitoringStats() for initial display...", "INFO");
        
        // Force immediate update
        QTimer::singleShot(100, this, [this]() {
            updateMonitoringStats();
        });
        
        addLog("✅ Disk monitoring started - will update every 5 seconds", "SUCCESS");
    }
}

void MainWindow::selectAllBackups() {
    for (int i = 0; i < backupTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(backupTable->cellWidget(i, 0));
        if (checkbox) {
            checkbox->setChecked(true);
        }
    }
}

void MainWindow::deleteSelectedBackups() {
    QStringList selectedBackups;
    QList<int> selectedRows;
    
    // Collect selected backup paths and row indices
    for (int i = 0; i < backupTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(backupTable->cellWidget(i, 0));
        if (checkbox && checkbox->isChecked()) {
            QTableWidgetItem *backupPathItem = backupTable->item(i, 2);
            if (backupPathItem) {
                selectedBackups.append(backupPathItem->text());
                selectedRows.append(i);
            }
        }
    }
    
    if (selectedBackups.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select backups to delete.");
        return;
    }
    
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, "Confirm Deletion",
        QString("Are you sure you want to delete %1 selected backup(s)?\nThis action cannot be undone.").arg(selectedBackups.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (confirm != QMessageBox::Yes) return;
    
    int deleted = 0;
    for (const QString &backupPath : selectedBackups) {
        try {
            if (QFile::exists(backupPath)) {
                if (QFile::remove(backupPath)) {
                    addLog(QString("Deleted backup: %1").arg(backupPath), "SUCCESS");
                    deleted++;
                } else {
                    addLog(QString("Failed to delete backup: %1").arg(backupPath), "ERROR");
                }
            } else {
                addLog(QString("Backup file not found: %1").arg(backupPath), "WARNING");
                deleted++; // Count as deleted since it's already gone
            }
        } catch (const std::exception &e) {
            addLog(QString("Error deleting backup %1: %2").arg(backupPath, e.what()), "ERROR");
        }
    }
    
    // Remove entries from backup index
    if (deleted > 0) {
        removeBackupsFromIndex(selectedBackups);
        updateBackupTable();
        QMessageBox::information(this, "Deletion Complete",
                                QString("Successfully deleted %1 of %2 backup(s).").arg(deleted).arg(selectedBackups.size()));
    }
}

void MainWindow::removeBackupsFromIndex(const QStringList &backupPaths) {
    auto backups = backupManager->loadBackupIndex();
    
    // Filter out the deleted backups
    QSet<QString> pathsToRemove(backupPaths.begin(), backupPaths.end());
    
    // Rewrite the index file without the deleted entries
    QString indexFile = QString::fromStdString(backupManager->getBackupDir()) + "/index.txt";
    QFile file(indexFile);
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        for (const auto& backup : backups) {
            QString backupPath = QString::fromStdString(backup.backupPath);
            if (!pathsToRemove.contains(backupPath)) {
                out << QString::fromStdString(backup.timestamp) << "|"
                    << QString::fromStdString(backup.originalPath) << "|"
                    << QString::fromStdString(backup.backupPath) << "|"
                    << backup.size << "\n";
            }
        }
        file.close();
    }
}