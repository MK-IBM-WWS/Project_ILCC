#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTabWidget>
#include "SysMonCore.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateSystemData();

private:
    Ui::MainWindow *ui;
    QTimer *updateTimer;
    Devices::PC& systemMonitor;

    QTabWidget* cpuInnerTabWidget;
    QTabWidget* ramInnerTabWidget;
    QTabWidget* networkInnerTabWidget;

    bool firstUpdate = true;

    void setupInnerTabs();
    void updateSystemTab();
    void updateCpuTabs();
    void updateRamTabs();
    void updateNetworkTabs();
    void updateAboutTab();
};

#endif // MAINWINDOW_H
