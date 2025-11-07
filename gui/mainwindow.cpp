// mainwindow.cpp  — corrected and updated

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

namespace fs = std::filesystem;

// ==================== ScanWorker ====================
ScanWorker::ScanWorker(const std::string &path, QObject *parent)
    : QThread(parent), scanPath(path) {}

void ScanWorker::run() {
    try {
        emit scanProgress(0);

        ScanResults results;
        std::map<QString, std::vector<FileDetail>> hashToFiles;

        // Count total files for progress estimation (protect against exceptions)
        int totalFiles = 0;
        try {
            for (const auto &entry : fs::recursive_directory_iterator(scanPath, fs::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file()) ++totalFiles;
            }
        } catch (...) {
            // If we can't count, set totalFiles to 0 — we'll still scan.
            totalFiles = 0;
        }

        int processedFiles = 0;
        int lastProgress = -1;
        QDateTime now = QDateTime::currentDateTime();
        QDateTime oldThreshold = now.addDays(-90);

        // If there are no files, still run a light pass to emit complete
        if (totalFiles == 0) {
            // Single pass — try to collect any accessible files (defensive)
            for (const auto &entry : fs::recursive_directory_iterator(scanPath, fs::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file()) {
                    try {
                        FileDetail detail;
                        detail.path = QString::fromStdString(entry.path().string());
                        detail.size = entry.file_size();

                        auto ftime = entry.last_write_time();
                        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                        ftime - fs::file_time_type::clock::now()
                                        + std::chrono::system_clock::now());
                        detail.lastModified = QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(sctp))
                                                .toString("yyyy-MM-dd hh:mm:ss");

                        detail.type = QString::fromStdString(entry.path().extension().string());
                        detail.isDuplicate = false;
                        detail.isOld = QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(sctp)) < oldThreshold;

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
                    } catch (...) { continue; }
                }
            }

            // duplicates
            DuplicateGroups duplicateGroups;
            for (auto &pair : hashToFiles) {
                if (pair.second.size() > 1) {
                    duplicateGroups.push_back(pair.second);
                    for (auto &file : results)
                        if (file.hash == pair.first)
                            file.isDuplicate = true;
                }
            }

            emit scanProgress(100);
            emit scanComplete(results, duplicateGroups);
            return;
        }

        // Normal scanning when totalFiles > 0
        for (const auto &entry : fs::recursive_directory_iterator(scanPath, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                try {
                    FileDetail detail;
                    detail.path = QString::fromStdString(entry.path().string());
                    detail.size = entry.file_size();

                    auto ftime = entry.last_write_time();
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                                    ftime - fs::file_time_type::clock::now()
                                    + std::chrono::system_clock::now());
                    detail.lastModified = QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(sctp))
                                            .toString("yyyy-MM-dd hh:mm:ss");

                    detail.type = QString::fromStdString(entry.path().extension().string());
                    detail.isDuplicate = false;
                    detail.isOld = QDateTime::fromSecsSinceEpoch(std::chrono::system_clock::to_time_t(sctp)) < oldThreshold;

                    // Hash calculation (only for sufficiently large files)
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
                    ++processedFiles;

                    // Update progress dynamically (avoid duplicate emissions)
                    int progress = static_cast<int>((processedFiles * 100.0) / totalFiles);
                    if (progress != lastProgress) {
                        emit scanProgress(progress);
                        lastProgress = progress;
                        // Allow the event loop to process UI updates so the progress bar repaints
                        QCoreApplication::processEvents();
                    }
                } catch (...) { continue; }
            }
        }

        // Detect duplicates
        DuplicateGroups duplicateGroups;
        for (auto &pair : hashToFiles) {
            if (pair.second.size() > 1) {
                duplicateGroups.push_back(pair.second);
                for (auto &file : results)
                    if (file.hash == pair.first)
                        file.isDuplicate = true;
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
    : QMainWindow(parent), isMonitoring(false), isScanning(false), scanWorker(nullptr)
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
    updateDiskInfo();

    setWindowTitle("Spacemate - Disk Space Manager");
    resize(1200, 800);
}

void MainWindow::setupConnections() {
    // File Analyzer
    connect(startScanBtn, &QPushButton::clicked, this, &MainWindow::startScan);
    connect(browseScanBtn, &QPushButton::clicked, this, &MainWindow::selectScanPath);

    // Cleanup
    connect(cleanTempBtn, &QPushButton::clicked, this, &MainWindow::cleanTempFiles);
    connect(cleanCacheBtn, &QPushButton::clicked, this, &MainWindow::cleanCache);
    connect(cleanupBtn, &QPushButton::clicked, this, &MainWindow::performCleanup);

    // Backup
    connect(browseSourceBtn, &QPushButton::clicked, this, &MainWindow::selectBackupSource);
    connect(browseDestBtn, &QPushButton::clicked, this, &MainWindow::selectBackupDestination);
    connect(createBackupBtn, &QPushButton::clicked, this, &MainWindow::createBackup);

    // Disk Monitoring
    connect(monitorBtn, &QPushButton::clicked, this, &MainWindow::toggleMonitoring);

    // Set up monitoring timer
    QTimer *monitorTimer = new QTimer(this);
    connect(monitorTimer, &QTimer::timeout, this, [this]() {
        if (isMonitoring) {
            updateDiskInfo();
            updateMonitoringStats();
        }
    });
    monitorTimer->start(5000); // Update every 5 seconds

    // Dashboard refresh on tab switch
    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 0) { // Dashboard tab
            updateDiskInfo();
            updateBackupTable();
        }
    });

    // Initial backup table population
    updateBackupTable();
}

MainWindow::~MainWindow() {
    if (scanWorker) {
        if (scanWorker->isRunning()) {
            scanWorker->requestInterruption();
            scanWorker->quit();
            scanWorker->wait();
        }
        scanWorker->deleteLater();
        scanWorker = nullptr;
    }
}

// ==================== UI Setup ====================
void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Title
    QLabel *titleLabel = new QLabel("Spacemate - Disk Space Manager");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #2563eb; padding: 10px;");
    mainLayout->addWidget(titleLabel);

    // Tab widget
    tabWidget = new QTabWidget();
    mainLayout->addWidget(tabWidget);

    createDashboardTab();
    createAnalyzerTab();
    createCleanupTab();
    createBackupTab();
    createMonitorTab();

    // Log display
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

// ==================== Dashboard Tab ====================
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

    storageProgressBar = new QProgressBar();
    storageProgressBar->setTextVisible(false);
    storageProgressBar->setMinimumHeight(30);

    usagePercentLabel = new QLabel("0%");
    usagePercentLabel->setAlignment(Qt::AlignCenter);
    QFont percentFont = usagePercentLabel->font();
    percentFont.setPointSize(14);
    percentFont.setBold(true);
    usagePercentLabel->setFont(percentFont);

    storageLayout->addLayout(infoLayout);
    storageLayout->addWidget(storageProgressBar);
    storageLayout->addWidget(usagePercentLabel);
    storageGroup->setLayout(storageLayout);

    // Quick Actions
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

// ==================== Analyzer Tab ====================
void MainWindow::createAnalyzerTab() {
    QWidget *analyzerWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(analyzerWidget);

    QGroupBox *scanGroup = new QGroupBox("Scan Directory");
    QHBoxLayout *scanLayout = new QHBoxLayout();

    scanPathInput = new QLineEdit();
    scanPathInput->setPlaceholderText("Enter path to scan...");

    browseScanBtn = new QPushButton("Browse");
    connect(browseScanBtn, &QPushButton::clicked, this, &MainWindow::selectScanPath);

    startScanBtn = new QPushButton("Start Scan");
    startScanBtn->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold;");
    connect(startScanBtn, &QPushButton::clicked, this, &MainWindow::startScan);

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

// ==================== Cleanup Tab ====================
void MainWindow::createCleanupTab() {
    QWidget *cleanupWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(cleanupWidget);

    QGroupBox *quickGroup = new QGroupBox("Quick Cleanup");
    QHBoxLayout *quickLayout = new QHBoxLayout();

    cleanTempBtn = new QPushButton("Clean Temp Files");
    cleanCacheBtn = new QPushButton("Clean Cache");
    QPushButton *customCleanBtn = new QPushButton("Custom Cleanup");

    connect(cleanTempBtn, &QPushButton::clicked, this, &MainWindow::cleanTempFiles);
    connect(cleanCacheBtn, &QPushButton::clicked, this, &MainWindow::cleanCache);

    quickLayout->addWidget(cleanTempBtn);
    quickLayout->addWidget(cleanCacheBtn);
    quickLayout->addWidget(customCleanBtn);
    quickGroup->setLayout(quickLayout);

    cleanupTable = new QTableWidget();
    cleanupTable->setColumnCount(4);
    cleanupTable->setHorizontalHeaderLabels({"Path", "Size", "Type", "Select"});
    cleanupTable->horizontalHeader()->setStretchLastSection(true);
    cleanupStatusLabel = new QLabel("Select items to clean");

    cleanupBtn = new QPushButton("Perform Cleanup");
    cleanupBtn->setStyleSheet("background-color: #dc2626; color: white; font-weight: bold; padding: 10px;");
    connect(cleanupBtn, &QPushButton::clicked, this, &MainWindow::performCleanup);

    layout->addWidget(quickGroup);
    layout->addWidget(cleanupTable);
    layout->addWidget(cleanupStatusLabel);
    layout->addWidget(cleanupBtn);

    tabWidget->addTab(cleanupWidget, "Cleanup");
}

// ==================== Backup Tab ====================
void MainWindow::createBackupTab() {
    QWidget *backupWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(backupWidget);

    // Source/Destination selection
    QGroupBox *createGroup = new QGroupBox("Create Backup");
    QVBoxLayout *createLayout = new QVBoxLayout();

    QHBoxLayout *sourceLayout = new QHBoxLayout();
    backupSourceInput = new QLineEdit();
    backupSourceInput->setPlaceholderText("Select source directory...");
    browseSourceBtn = new QPushButton("Browse");
    connect(browseSourceBtn, &QPushButton::clicked, this, &MainWindow::selectBackupSource);
    sourceLayout->addWidget(new QLabel("Source:"));
    sourceLayout->addWidget(backupSourceInput);
    sourceLayout->addWidget(browseSourceBtn);

    QHBoxLayout *destLayout = new QHBoxLayout();
    backupDestInput = new QLineEdit();
    backupDestInput->setPlaceholderText("Select destination directory...");
    browseDestBtn = new QPushButton("Browse");
    connect(browseDestBtn, &QPushButton::clicked, this, &MainWindow::selectBackupDestination);
    destLayout->addWidget(new QLabel("Destination:"));
    destLayout->addWidget(backupDestInput);
    destLayout->addWidget(browseDestBtn);

    createBackupBtn = new QPushButton("Create Backup");
    createBackupBtn->setStyleSheet("background-color: #7c3aed; color: white; font-weight: bold;");
    connect(createBackupBtn, &QPushButton::clicked, this, &MainWindow::createBackup);

    createLayout->addLayout(sourceLayout);
    createLayout->addLayout(destLayout);
    createLayout->addWidget(createBackupBtn);
    createGroup->setLayout(createLayout);

    // Backup table
    QGroupBox *listGroup = new QGroupBox("Existing Backups");
    QVBoxLayout *listLayout = new QVBoxLayout();
    backupTable = new QTableWidget();
    backupTable->setColumnCount(4);
    backupTable->setHorizontalHeaderLabels({"Source", "Destination", "Date", "Action"});
    backupTable->horizontalHeader()->setStretchLastSection(true);
    listLayout->addWidget(backupTable);
    listGroup->setLayout(listLayout);

    layout->addWidget(createGroup);
    layout->addWidget(listGroup);
    tabWidget->addTab(backupWidget, "Backup");
}

// ==================== Disk Monitor Tab ====================
void MainWindow::createMonitorTab() {
    QWidget *monitorWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(monitorWidget);

    monitorStatusLabel = new QLabel("Monitoring stopped");
    monitorStatusLabel->setStyleSheet("color: #facc15; font-weight: bold;");

    monitorBtn = new QPushButton("Start Monitoring");
    connect(monitorBtn, &QPushButton::clicked, this, &MainWindow::toggleMonitoring);

    layout->addWidget(monitorStatusLabel);
    layout->addWidget(monitorBtn);
    layout->addStretch();

    tabWidget->addTab(monitorWidget, "Monitor");
}

// ==================== Logging ====================
void MainWindow::addLog(const QString &message, const QString &status) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    logDisplay->append(QString("[%1] [%2] %3").arg(timestamp, status, message));
}

void MainWindow::clearLogs() {
    logDisplay->clear();
}

// ==================== Disk Info ====================
void MainWindow::updateDiskInfo() {
    auto diskData = diskMonitor->getDiskInfo("/"); // Returns vector<pair<string,long long>>

    long long totalBytes = 0, usedBytes = 0, freeBytes = 0;

    for (const auto& item : diskData) {
        if (item.first == "total") totalBytes = item.second;
        else if (item.first == "used") usedBytes = item.second;
        else if (item.first == "free") freeBytes = item.second;
    }

    double totalGB = totalBytes / (1024.0 * 1024.0 * 1024.0);
    double usedGB = usedBytes / (1024.0 * 1024.0 * 1024.0);
    double freeGB = freeBytes / (1024.0 * 1024.0 * 1024.0);

    totalSpaceLabel->setText(QString("Total: %1 GB").arg(totalGB, 0, 'f', 2));
    usedSpaceLabel->setText(QString("Used: %1 GB").arg(usedGB, 0, 'f', 2));
    freeSpaceLabel->setText(QString("Free: %1 GB").arg(freeGB, 0, 'f', 2));

    int percentUsed = (totalGB > 0) ? static_cast<int>((usedGB * 100.0) / totalGB) : 0;
    storageProgressBar->setValue(percentUsed);
    usagePercentLabel->setText(QString("%1%").arg(percentUsed));
}

void MainWindow::refreshDashboard() {
    updateDiskInfo();
    addLog("Dashboard refreshed", "INFO");
}

// ==================== Slots for Scan ====================
void MainWindow::selectScanPath() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Directory to Scan");
    if (!path.isEmpty()) {
        // Convert Windows path to WSL path if needed
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

    QString path = scanPathInput->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, "Invalid Path", "Please enter a valid path to scan.");
        return;
    }

    // Prepare progress bar & status
    scanProgressBar->setRange(0, 100);
    scanProgressBar->setValue(0);
    scanProgressBar->setTextVisible(true);
    scanProgressBar->setVisible(true);
    scanStatusLabel->setText("Scanning...");
    fileTable->setRowCount(0);

    // Make sure any previous worker is cleaned up
    if (scanWorker) {
        if (scanWorker->isRunning()) {
            scanWorker->requestInterruption();
            scanWorker->quit();
            scanWorker->wait();
        }
        scanWorker->deleteLater();
        scanWorker = nullptr;
    }

    scanWorker = new ScanWorker(path.toStdString(), this);
    // Use queued connections for cross-thread delivery
    connect(scanWorker, &ScanWorker::scanProgress, this, [this](int p){ scanProgressBar->setValue(p); }, Qt::QueuedConnection);
    connect(scanWorker, &ScanWorker::scanComplete, this, &MainWindow::onScanComplete, Qt::QueuedConnection);
    connect(scanWorker, &ScanWorker::scanError, this, &MainWindow::onScanError, Qt::QueuedConnection);

    isScanning = true;
    scanWorker->start();
    addLog(QString("Started scanning %1").arg(path), "INFO");
}

void MainWindow::onScanComplete(const ScanResults &results, const DuplicateGroups &duplicates) {
    // Populate fileTable with results
    fileTable->setRowCount(results.size());
    long long totalSize = 0;
    int fileCount = results.size();
    int duplicateCount = 0;
    int oldFileCount = 0;

    // Setup columns
    fileTable->setColumnCount(6);
    fileTable->setHorizontalHeaderLabels({"Path", "Size", "Type", "Last Modified", "Status", "Action"});

    for (size_t i = 0; i < results.size(); ++i) {
        fileTable->setItem(i, 0, new QTableWidgetItem(results[i].path));

        long long sizeBytes = results[i].size;
        totalSize += sizeBytes;
        QString sizeStr;
        if (sizeBytes < 1024) sizeStr = QString("%1 B").arg(sizeBytes);
        else if (sizeBytes < 1024LL * 1024LL) sizeStr = QString("%1 KB").arg(sizeBytes / 1024.0, 0, 'f', 2);
        else if (sizeBytes < 1024LL * 1024LL * 1024LL) sizeStr = QString("%1 MB").arg(sizeBytes / (1024.0 * 1024.0), 0, 'f', 2);
        else sizeStr = QString("%1 GB").arg(sizeBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
        fileTable->setItem(i, 1, new QTableWidgetItem(sizeStr));

        fileTable->setItem(i, 2, new QTableWidgetItem(results[i].type.isEmpty() ? "File" : results[i].type));
        fileTable->setItem(i, 3, new QTableWidgetItem(results[i].lastModified));

        QString status;
        if (results[i].isDuplicate) { status = "Duplicate"; ++duplicateCount; }
        if (results[i].isOld) { status += status.isEmpty() ? "Old" : ", Old"; ++oldFileCount; }
        if (status.isEmpty()) status = "Normal";

        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        if (results[i].isDuplicate || results[i].isOld) statusItem->setBackground(QColor(255,200,200));
        fileTable->setItem(i, 4, statusItem);

        QPushButton *actionBtn = new QPushButton("Delete");
        actionBtn->setStyleSheet((results[i].isDuplicate || results[i].isOld) ? "background-color: #dc2626; color: white;" : "background-color: #666666; color: white;");
        fileTable->setCellWidget(i, 5, actionBtn);
        connect(actionBtn, &QPushButton::clicked, this, [this, p = results[i].path]() { deleteFileFromPath(p); });
    }

    fileTable->resizeColumnToContents(1);
    fileTable->resizeColumnToContents(2);
    fileTable->resizeColumnToContents(3);

    QString totalSizeStr;
    if (totalSize < 1024LL * 1024LL * 1024LL) totalSizeStr = QString("%1 MB").arg(totalSize / (1024.0 * 1024.0), 0, 'f', 2);
    else totalSizeStr = QString("%1 GB").arg(totalSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);

    QString summary = QString("Scan complete:\n");
    summary += QString("- Total files: %1\n").arg(fileCount);
    summary += QString("- Total size: %1\n").arg(totalSizeStr);
    if (duplicateCount > 0) summary += QString("- Duplicate files: %1\n").arg(duplicateCount);
    if (oldFileCount > 0) summary += QString("- Old files (>90 days): %1\n").arg(oldFileCount);

    scanStatusLabel->setText(summary);

    // Progress bar finalization
    scanProgressBar->setValue(100);
    scanProgressBar->setVisible(false);

    isScanning = false;

    // Cleanup table auto-fill (Option A)
    cleanupTable->setRowCount(0);
    int cleanupRow = 0;
    for (const auto &file : results) {
        if (file.isDuplicate || file.isOld) {
            cleanupTable->insertRow(cleanupRow);
            cleanupTable->setItem(cleanupRow, 0, new QTableWidgetItem(file.path));
            // size cell
            long long sb = file.size;
            QString sstr;
            if (sb < 1024) sstr = QString("%1 B").arg(sb);
            else if (sb < 1024LL*1024LL) sstr = QString("%1 KB").arg(sb/1024.0,0,'f',2);
            else sstr = QString("%1 MB").arg(sb/(1024.0*1024.0),0,'f',2);
            cleanupTable->setItem(cleanupRow, 1, new QTableWidgetItem(sstr));
            cleanupTable->setItem(cleanupRow, 2, new QTableWidgetItem(file.type.isEmpty() ? "File" : file.type));

            // checkbox widget in cell 3
            QWidget *boxContainer = new QWidget();
            QHBoxLayout *hl = new QHBoxLayout(boxContainer);
            hl->setContentsMargins(0,0,0,0);
            QCheckBox *cb = new QCheckBox();
            cb->setChecked(true);
            hl->addWidget(cb);
            boxContainer->setLayout(hl);
            cleanupTable->setCellWidget(cleanupRow, 3, boxContainer);

            ++cleanupRow;
        }
    }
    cleanupStatusLabel->setText(QString("Found %1 items eligible for cleanup").arg(cleanupRow));

    // Suggest using Cleanup tab
    if (duplicateCount > 0 || oldFileCount > 0) {
        QMessageBox::information(this, "Cleanup Suggestion",
                                 QString("Found %1 duplicate files and %2 old files.\nConsider using the Cleanup tab to free up space.")
                                 .arg(duplicateCount).arg(oldFileCount));
    }

    addLog(QString("Scan completed: %1 files (%2 duplicates, %3 old) found, %4 total size")
           .arg(fileCount).arg(duplicateCount).arg(oldFileCount).arg(totalSizeStr), "SUCCESS");

    // teardown worker
    if (scanWorker) {
        scanWorker->deleteLater();
        scanWorker = nullptr;
    }
}

void MainWindow::onScanError(const QString &error) {
    QMessageBox::critical(this, "Scan Error", error);
    scanStatusLabel->setText("Scan failed");
    scanProgressBar->setVisible(false);
    isScanning = false;
    addLog(QString("Scan failed: %1").arg(error), "ERROR");

    if (scanWorker) {
        scanWorker->deleteLater();
        scanWorker = nullptr;
    }
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

                // Remove from fileTable (search rows)
                for (int i = 0; i < fileTable->rowCount(); ++i) {
                    QTableWidgetItem *it = fileTable->item(i, 0);
                    if (it && it->text() == path) {
                        fileTable->removeRow(i);
                        break;
                    }
                }

                // Also remove any matching row in cleanupTable
                for (int i = 0; i < cleanupTable->rowCount(); ++i) {
                    QTableWidgetItem *it = cleanupTable->item(i, 0);
                    if (it && it->text() == path) {
                        cleanupTable->removeRow(i);
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

// ==================== Cleanup Slots ====================
void MainWindow::cleanTempFiles() {
    QMessageBox::StandardButton confirm = QMessageBox::question(this,
        "Clean Temporary Files",
        "This will clean temporary files. Do you want to continue?",
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
            progress.setValue((progress.value() + 1) % 100);
            QThread::msleep(80);
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
        "This will clean cache files. Do you want to continue?",
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
            progress.setValue((progress.value() + 1) % 100);
            QThread::msleep(80);
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

            // Create backup path under backupManager path with timestamp
            QString backupRoot = QString::fromStdString(backupManager->getBackupDir());
            QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
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

    cleanupStatusLabel->setText("Cleanup completed successfully.");
    QMessageBox::information(this, "Cleanup Complete",
                             QString("Cleaned %1 of %2 items successfully.").arg(cleaned).arg(selectedPaths.size()));

    cleanupBtn->setEnabled(true);
    cleanTempBtn->setEnabled(true);
    cleanCacheBtn->setEnabled(true);
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

        backupTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(backup.originalPath)));
        backupTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(backup.backupPath)));
        backupTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(backup.timestamp)));

        QPushButton *restoreBtn = new QPushButton("Restore");
        restoreBtn->setStyleSheet("background-color: #7c3aed; color: white;");

        connect(restoreBtn, &QPushButton::clicked, [this, backup]() {
            QMessageBox::StandardButton confirm = QMessageBox::question(this,
                "Restore Backup",
                QString("Do you want to restore the backup from %1?").arg(QString::fromStdString(backup.timestamp)),
                QMessageBox::Yes | QMessageBox::No);

            if (confirm == QMessageBox::Yes) {
                QProgressDialog progress("Restoring backup...", "Cancel", 0, 100, this);
                progress.setWindowModality(Qt::WindowModal);
                progress.setMinimumDuration(0);

                QFuture<bool> future = QtConcurrent::run([this, &backup]() {
                    try {
                        return fs::copy_file(backup.backupPath, backup.originalPath, fs::copy_options::overwrite_existing);
                    } catch (...) { return false; }
                });

                while (!future.isFinished()) {
                    QCoreApplication::processEvents();
                    progress.setValue((progress.value() + 1) % 100);
                    QThread::msleep(80);
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
        backupTable->setCellWidget(row, 3, btnContainer);
    }

    backupTable->resizeColumnsToContents();
}

// FIX 3: Handle backup return value properly (string instead of bool)
void MainWindow::createBackup() {
    QString src = backupSourceInput->text();
    QString dest = backupDestInput->text();

    if (src.isEmpty() || dest.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Source and destination must be selected.");
        return;
    }

    // Ensure paths are in WSL format
    if (!src.startsWith("/")) src = convertToWSLPath(src);
    if (!dest.startsWith("/")) dest = convertToWSLPath(dest);

    // Create destination directory if it doesn't exist
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
        progress.setValue((progress.value() + 1) % 100);
        QThread::msleep(80);
    }

    std::string backupPath = future.result();
    progress.setValue(100);

    if (!backupPath.empty()) {
        updateBackupTable();
        addLog(QString("Backup created: %1").arg(QString::fromStdString(backupPath)), "SUCCESS");
        QMessageBox::information(this, "Backup Complete", "Backup has been created successfully.");
    } else {
        addLog(QString("Failed to create backup from %1 to %2").arg(src, dest), "ERROR");
        QMessageBox::critical(this, "Backup Failed", "Failed to create the backup.");
    }
}

// ==================== Monitoring Slots ====================
void MainWindow::updateMonitoringStats() {
    if (!isMonitoring) return;

    // Get largest directories
    auto dirs = diskMonitor->getLargestDirectories("/", 5);

    // Update disk usage
    updateDiskInfo();

    // Format directory information
    QString stats = "Monitoring active...\n\n";
    stats += "Largest Directories:\n";
    stats += "-------------------\n";
    for (const auto& dir : dirs) {
        stats += QString("%1: %2\n")
            .arg(QString::fromStdString(dir.first))
            .arg(Utils::formatSize(dir.second).c_str());
    }

    // Check disk space and add warnings
    QStorageInfo storage("/");
    double freePercent = 0.0;
    if (storage.isValid() && storage.bytesTotal() > 0)
        freePercent = (storage.bytesAvailable() * 100.0) / storage.bytesTotal();

    if (freePercent < 10.0) {
        stats += "\nWARNING: Low disk space!\n";
        stats += QString("Only %.1f%% space remaining\n").arg(freePercent);
    }

    monitorStatusLabel->setText(stats);
    monitorStatusLabel->setStyleSheet("color: " + QString(freePercent < 10 ? "#dc2626" : "#059669") + ";");
}

void MainWindow::toggleMonitoring() {
    if (isMonitoring) {
        diskMonitor->stopMonitoring();
        monitorBtn->setText("Start Monitoring");
        monitorStatusLabel->setText("Monitoring stopped");
        addLog("Disk monitoring stopped", "INFO");
    } else {
        diskMonitor->startMonitoring();
        monitorBtn->setText("Stop Monitoring");
        updateMonitoringStats();
        addLog("Disk monitoring started", "INFO");
    }
    isMonitoring = !isMonitoring;
}
