#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <memory>
#include <vector>
#include <string>

#include "../include/backup_manager.h"
#include "../include/utils.h"
#include "../include/cleanup_manager.h"
#include "../include/disk_monitor.h"
#include "../include/file_analyzer.h"
#include "custom_types.h"
#include "scan_results.h"

Q_DECLARE_METATYPE(FileDetail)
Q_DECLARE_METATYPE(ScanResults)
Q_DECLARE_METATYPE(DuplicateGroups)

// ==================== ScanWorker ====================
class ScanWorker : public QThread {
    Q_OBJECT
public:
    explicit ScanWorker(const std::string &path, QObject *parent = nullptr);
signals:
    void scanProgress(int percent);
    void scanComplete(const ScanResults &results, const DuplicateGroups &duplicates);
    void scanError(const QString &error);
protected:
    void run() override;
private:
    std::string scanPath;
};

// ==================== MainWindow ====================
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // Helper methods
    QString convertToWSLPath(const QString &windowsPath);

    // Core managers
    std::unique_ptr<BackupManager> backupManager;
    std::unique_ptr<CleanupManager> cleanupManager;
    std::unique_ptr<DiskMonitor> diskMonitor;
    std::unique_ptr<FileAnalyzer> fileAnalyzer;

    // GUI Components
    QTabWidget *tabWidget;

    // Dashboard
    QLabel *totalSpaceLabel;
    QLabel *usedSpaceLabel;
    QLabel *freeSpaceLabel;
    QProgressBar *storageProgressBar;
    QLabel *usagePercentLabel;

    // Logs
    QTextEdit *logDisplay;

    // Analyzer Tab
    QLineEdit *scanPathInput;
    QPushButton *browseScanBtn;
    QPushButton *startScanBtn;
    QProgressBar *scanProgressBar;
    QLabel *scanStatusLabel;
    QTableWidget *fileTable;

    ScanWorker *scanWorker;
    bool isScanning;

    // Cleanup Tab
    QPushButton *cleanTempBtn;
    QPushButton *cleanCacheBtn;
    QPushButton *cleanupBtn;
    QTableWidget *cleanupTable;
    QLabel *cleanupStatusLabel;

    // Backup Tab
    QLineEdit *backupSourceInput;
    QLineEdit *backupDestInput;
    QPushButton *browseSourceBtn;
    QPushButton *browseDestBtn;
    QPushButton *createBackupBtn;
    QTableWidget *backupTable;

    // Monitor Tab
    QLabel *monitorStatusLabel;
    QPushButton *monitorBtn;
    bool isMonitoring;

    // ==================== UI Setup ====================
    void setupUI();
    void setupConnections();
    void createDashboardTab();
    void createAnalyzerTab();
    void createCleanupTab();
    void createBackupTab();
    void createMonitorTab();

    // ==================== Helpers ====================
    void addLog(const QString &message, const QString &status);
    void clearLogs();
    void updateDiskInfo();
    void refreshDashboard();
    void updateMonitoringStats();
    void updateBackupTable();

private slots:
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

    // Backup
    void selectBackupSource();
    void selectBackupDestination();
    void createBackup();

    // Disk Monitoring
    void toggleMonitoring();
};

#endif // MAINWINDOW_H
