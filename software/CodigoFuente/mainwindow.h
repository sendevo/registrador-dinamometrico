#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define EOF_CODE "##EOF##"
#define EOL_CODE "%%EOL%%"
#define EOD_CODE "$$EOD$$"
#define EOW_CODE "&&EOW&&"

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QComboBox>
#include <QSpinBox>
#include <QToolBar>
#include <QTimer>
#include <QStatusBar>
#include <QErrorMessage>
#include <QMessageBox>
#include <QDate>
#include <QTime>
#include <QProgressBar>

#include "qextserialport.h"
#include "qextserialenumerator.h"

#include <iostream>



class LoadCellMonitor : public QWidget
{
    Q_OBJECT
public:
    explicit LoadCellMonitor(QWidget *parent = 0);
    ~LoadCellMonitor();

    void updateBars(int bar1, int bar2, int bar3, int bar4);

signals:
    void sampleRequestSgn();

private slots:
    void sampleRequest();

private:
    QTimer *timer;
    QProgressBar *progressBarCh1, *progressBarCh2, *progressBarCh3, *progressBarCh4;
};




class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void downloadBtnClicked();
    void deleteBtnClicked();
    void setTimeBtnClicked();
    void cellStateBtnClicked();
    void connectBtnClicked();
    void requestFileList();
    void sampleRequest();
    void setRTCTime(QString amdhms);

    void onDeviceDiscovered();
    void onDeviceRemoved();
    void onDataAvailable();

private:
    void showFileList();
    void downloadFileQueue();
    void deleteFileQueue();
    void saveFileReceived();
    void loadPortConfiguration();

    int bytesDownloaded, bytesToDownload;

    QTableWidget *filesTable;
    QComboBox *portSelect;
    QPushButton *downloadBtn, *deleteBtn, *setTimeBtn, *cellStateBtn;
    QStatusBar *statusBar;
    QErrorMessage *errorMessage;
    QextSerialPort *serialPort;
    QStringList filesQueue, filesNameQueue, sizesQueue, strListReceived;
    QString strReceived, logFilesDir;
    QProgressBar *progressBar;
    LoadCellMonitor *loadCellMonitor;
};




class TimeSettingWindow : public QWidget
{
    Q_OBJECT
public:
    explicit TimeSettingWindow(QWidget *parent = 0);
    ~TimeSettingWindow();

signals:
    void okBtnClickedSgn(QString);

private slots:
    void okBtnClicked();
    void cancelBtnClicked();

private:
    QComboBox *monthSelect;
    QSpinBox *yearSelect, *daySelect, *hourSelect, *minuteSelect, *secondSelect;
};





#endif // MAINWINDOW_H
