
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QThread>
#include <QCheckBox>
#include <memory>
#include <vector>
#include "../include/backup_manager.h"
#include "../include/cleanup_manager.h"
#include "../include/disk_monitor.h"
#include "../include/file_analyzer.h"

// Forward declarations
struct FileDetail {
    QString path;
    long long size;
    QString lastModified;
    QString type;
    QString hash;
    bool isDuplicate;
    bool isOld;
};

using ScanResults = std::vector<FileDetail>;
using DuplicateGroups = std::vector<std::vector<FileDetail>>;

Q_DECLARE_METATYPE(FileDetail)
Q_DECLARE_METATYPE(ScanResults)
Q_DECLARE_METATYPE(DuplicateGroups)

// Worker thread for scanning
class ScanWorker : public QThread {
    Q_OBJECT

public:
    ScanWorker(const std::string &path, QObject *parent = nullptr);
    void run() override;

signals:
    void scanProgress(int percent);
    void scanComplete(const ScanResults &results, const DuplicateGroups &duplicates);
    void scanError(const QString &error);

private:
    std::string scanPath;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Dashboard
    void updateDiskInfo();
    void refreshDashboard();

    // File Analyzer
    void selectScanPath();
    void startScan();
    void onScanComplete(const ScanResults &results, const DuplicateGroups &duplicates);
    void onScanError(const QString &error);
    void deleteFileFromTable(int row);
    void deleteFileFromPath(const QString &path);

    // Cleanup
    void cleanTempFiles();
    void cleanCache();
    void performCleanup();
    void populateCleanupTable(const ScanResults &results);  // NEW: Added function

    // Backup
    void selectBackupSource();
    void selectBackupDestination();
    void createBackup();
    void updateBackupTable();
    void selectAllBackups();
    void deleteSelectedBackups();

    // Monitoring
    void toggleMonitoring();
    void updateMonitoringStats();

    // Logging
    void addLog(const QString &message, const QString &status);
    void clearLogs();

private:
    void setupUI();
    void setupConnections();
    void createDashboardTab();
    void createAnalyzerTab();
    void createCleanupTab();
    void createBackupTab();
    void createMonitorTab();
    QString convertToWSLPath(const QString &windowsPath);
    void removeBackupsFromIndex(const QStringList &backupPaths);

    // UI Components
    QTabWidget *tabWidget;

    // Dashboard
    QLabel *totalSpaceLabel;
    QLabel *usedSpaceLabel;
    QLabel *freeSpaceLabel;
    QLabel *usagePercentLabel;

    // File Analyzer
    QLineEdit *scanPathInput;
    QPushButton *browseScanBtn;
    QPushButton *startScanBtn;
    QProgressBar *scanProgressBar;
    QLabel *scanStatusLabel;
    QTableWidget *fileTable;

    // Cleanup
    QPushButton *cleanTempBtn;
    QPushButton *cleanCacheBtn;
    QPushButton *customCleanBtn;
    QPushButton *cleanupBtn;
    QTableWidget *cleanupTable;
    QLabel *cleanupStatusLabel;

    // Backup
    QLineEdit *backupSourceInput;
    QLineEdit *backupDestInput;
    QPushButton *browseSourceBtn;
    QPushButton *browseDestBtn;
    QPushButton *createBackupBtn;
    QTableWidget *backupTable;

    // Monitoring
    QPushButton *monitorBtn;
    QLabel *monitorStatusLabel;

    // Log
    QTextEdit *logDisplay;

    // Backend Managers
    std::unique_ptr<BackupManager> backupManager;
    std::unique_ptr<CleanupManager> cleanupManager;
    std::unique_ptr<DiskMonitor> diskMonitor;
    std::unique_ptr<FileAnalyzer> fileAnalyzer;

    // Worker thread
    ScanWorker *scanWorker;

    // State
    bool isMonitoring;
    bool isScanning;
    QString lastScannedPath;  // Track the last scanned/analyzed path for monitoring
};

#endif // MAINWINDOW_H