#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QProgressBar>
#include <QLineEdit>
#include <QFormLayout>
#include <QListWidget>
#include <set>
#include <unordered_set>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , systemMonitor(Devices::PC::GetInstance())
{
    ui->setupUi(this);
    setWindowTitle("System Monitor");

    ui->formLayoutWidget_2->hide();
    ui->formLayoutWidget_3->hide();
    ui->formLayoutWidget_4->hide();

    setupInnerTabs();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateSystemData);
    updateTimer->start(1000);

    firstUpdate = true;
    updateSystemData();
}

void MainWindow::setupInnerTabs()
{
    cpuInnerTabWidget = new QTabWidget(ui->tab_3);
    cpuInnerTabWidget->setGeometry(10, 20, 701, 481);
    cpuInnerTabWidget->setTabPosition(QTabWidget::West);

    ramInnerTabWidget = new QTabWidget(ui->tab_4);
    ramInnerTabWidget->setGeometry(10, 20, 701, 481);
    ramInnerTabWidget->setTabPosition(QTabWidget::West);

    networkInnerTabWidget = new QTabWidget(ui->tab_5);
    networkInnerTabWidget->setGeometry(10, 20, 701, 481);
    networkInnerTabWidget->setTabPosition(QTabWidget::West);
}

void MainWindow::updateSystemData()
{
    systemMonitor.UpdateData();

    // Сохраняем текущие индексы вкладок
    int cpuTabIndex = cpuInnerTabWidget->currentIndex();
    int ramTabIndex = ramInnerTabWidget->currentIndex();
    int networkTabIndex = networkInnerTabWidget->currentIndex();

    // Обновляем все вкладки
    updateSystemTab();
    updateCpuTabs();
    updateRamTabs();
    updateNetworkTabs();

    // Восстанавливаем индексы вкладок
    if (cpuTabIndex >= 0 && cpuTabIndex < cpuInnerTabWidget->count()) {
        cpuInnerTabWidget->setCurrentIndex(cpuTabIndex);
    }
    if (ramTabIndex >= 0 && ramTabIndex < ramInnerTabWidget->count()) {
        ramInnerTabWidget->setCurrentIndex(ramTabIndex);
    }
    if (networkTabIndex >= 0 && networkTabIndex < networkInnerTabWidget->count()) {
        networkInnerTabWidget->setCurrentIndex(networkTabIndex);
    }

    if (firstUpdate) {
        updateAboutTab();
        firstUpdate = false;
    }
}

void MainWindow::updateSystemTab()
{
    // Hostname
    ui->hostnameLabel->setText(QString::fromStdString(systemMonitor.GetHostname()));

    // Uptime
    Devices::Uptime uptime = systemMonitor.GetUptime();
    ui->uptimeLabel->setText(
        QString("%1d %2h %3min")
            .arg(uptime.days)
            .arg(uptime.hours, 2, 10, QLatin1Char('0'))
            .arg(uptime.minutes, 2, 10, QLatin1Char('0')));

    // CPU Usage
    double cpuUsage = systemMonitor.GetCPUUse();
    ui->cpuUsageBar->setValue(static_cast<int>(cpuUsage));
    ui->cpuUsageBar->setFormat(QString::number(cpuUsage, 'f', 1) + "%");

    // RAM Usage
    int totalRAM = systemMonitor.GetRAMVolume();
    int usedRAM = systemMonitor.GetUsedRAMVolume();
    double ramPercent = (totalRAM > 0) ? (usedRAM * 100.0) / totalRAM : 0.0;
    ui->ramUsageBar->setValue(static_cast<int>(ramPercent));
    ui->ramUsageBar->setFormat(QString::number(ramPercent, 'f', 1) + "%");

    // GPU
    auto gpus = systemMonitor.GetGPU();
    QString gpuText;
    if (gpus.empty()) {
        gpuText = "-";
    } else {
        for (const auto& gpu : gpus) {
            if (!gpuText.isEmpty()) gpuText += "\n";
            gpuText += QString::fromStdString(gpu.empty() ? "-" : gpu);
        }
    }
    ui->gpuLabel->setText(gpuText);

    // Network Controllers
    auto nics = systemMonitor.GetNIControllers();
    QString nicText;
    if (nics.empty()) {
        nicText = "-";
    } else {
        std::set<std::string> uniqueNics;
        for (const auto& nic : nics) {
            uniqueNics.insert(nic.empty() ? "-" : nic);
        }
        for (const auto& nic : uniqueNics) {
            if (!nicText.isEmpty()) nicText += "\n";
            nicText += QString::fromStdString(nic);
        }
    }
    ui->nicLabel->clear();
    ui->nicLabel->addItem(nicText);
}

void MainWindow::updateCpuTabs()
{
    auto cpus = systemMonitor.GetCPU();

    // Очищаем старые вкладки
    while (cpuInnerTabWidget->count() > 0) {
        QWidget* widget = cpuInnerTabWidget->widget(0);
        cpuInnerTabWidget->removeTab(0);
        delete widget;
    }

    if (cpus.empty()) {
        QWidget* tab = new QWidget();
        QFormLayout* layout = new QFormLayout(tab);
        QLabel* noCpuLabel = new QLabel("No CPU information available", tab);
        layout->addWidget(noCpuLabel);
        cpuInnerTabWidget->addTab(tab, "No CPU");
        return;
    }

    // Создаём новые вкладки с актуальными данными
    for (size_t i = 0; i < cpus.size(); ++i) {
        const auto& cpu = cpus[i];
        QWidget* tab = new QWidget();
        QFormLayout* layout = new QFormLayout(tab);

        // Создаём и настраиваем все поля
        auto addReadOnlyField = [&](const QString& label, const QString& value) {
            QLineEdit* field = new QLineEdit(tab);
            field->setReadOnly(true);
            field->setFocusPolicy(Qt::NoFocus);
            field->setText(value);
            layout->addRow(label + ":", field);
            return field;
        };

        addReadOnlyField("Name", QString::fromStdString(cpu.GetName()));
        addReadOnlyField("Cores", QString::number(cpu.GetCores()));
        addReadOnlyField("Threads", QString::number(cpu.GetThreats()));
        addReadOnlyField("Max Speed", QString::fromStdString(cpu.GetMaxSpeed()));
        addReadOnlyField("Socket", QString::fromStdString(cpu.GetSocket()));
        addReadOnlyField("L1 Cache", QString::fromStdString(cpu.GetL1Cache()));
        addReadOnlyField("L2 Cache", QString::fromStdString(cpu.GetL2Cache()));
        addReadOnlyField("L3 Cache", QString::fromStdString(cpu.GetL3Cache()));
        addReadOnlyField("Temperature", QString("%1°C").arg(cpu.GetTemperature()));

        cpuInnerTabWidget->addTab(tab, QString("CPU %1").arg(i+1));
    }
}

void MainWindow::updateRamTabs()
{
    auto rams = systemMonitor.GetRam();

    // Очищаем старые вкладки
    while (ramInnerTabWidget->count() > 0) {
        QWidget* widget = ramInnerTabWidget->widget(0);
        ramInnerTabWidget->removeTab(0);
        delete widget;
    }

    if (rams.empty()) {
        QWidget* tab = new QWidget();
        QFormLayout* layout = new QFormLayout(tab);
        QLabel* noRamLabel = new QLabel("No RAM information available", tab);
        layout->addWidget(noRamLabel);
        ramInnerTabWidget->addTab(tab, "No RAM");
        return;
    }

    // Создаём новые вкладки
    for (size_t i = 0; i < rams.size(); ++i) {
        const auto& ram = rams[i];
        QWidget* tab = new QWidget();
        QFormLayout* layout = new QFormLayout(tab);

        auto addReadOnlyField = [&](const QString& label, const QString& value) {
            QLineEdit* field = new QLineEdit(tab);
            field->setReadOnly(true);
            field->setFocusPolicy(Qt::NoFocus);
            field->setText(value);
            layout->addRow(label + ":", field);
        };

        addReadOnlyField("Name", QString::fromStdString(ram.GetName()));
        addReadOnlyField("Size", QString::fromStdString(ram.GetSize()));
        addReadOnlyField("Type", QString::fromStdString(ram.GetType()));
        addReadOnlyField("Speed", QString::fromStdString(ram.GetSpeed()));
        addReadOnlyField("Manufacturer", QString::fromStdString(ram.GetManufacturer()));
        addReadOnlyField("Form Factor", QString::fromStdString(ram.GetFormFactor()));
        addReadOnlyField("Channel", QString::fromStdString(ram.GetChannel()));
        addReadOnlyField("Rank", QString::number(ram.GetRank()));

        ramInnerTabWidget->addTab(tab, QString("RAM %1").arg(i+1));
    }
}

void MainWindow::updateNetworkTabs()
{
    auto interfaces = systemMonitor.GetNIs();

    // Удаляем дубликаты интерфейсов
    std::unordered_set<std::string> seenInterfaces;
    std::vector<Devices::NetworkInterface> uniqueInterfaces;

    for (const auto& net : interfaces) {
        if (seenInterfaces.insert(net.GetName()).second) {
            uniqueInterfaces.push_back(net);
        }
    }

    // Очищаем старые вкладки
    while (networkInnerTabWidget->count() > 0) {
        QWidget* widget = networkInnerTabWidget->widget(0);
        networkInnerTabWidget->removeTab(0);
        delete widget;
    }

    if (uniqueInterfaces.empty()) {
        QWidget* tab = new QWidget();
        QFormLayout* layout = new QFormLayout(tab);
        QLabel* noInterfacesLabel = new QLabel("No network interfaces found", tab);
        layout->addWidget(noInterfacesLabel);
        networkInnerTabWidget->addTab(tab, "No Interfaces");
    } else {
        for (size_t i = 0; i < uniqueInterfaces.size(); ++i) {
            const auto& net = uniqueInterfaces[i];
            QWidget* tab = new QWidget();
            QFormLayout* layout = new QFormLayout(tab);

            auto addReadOnlyField = [&](const QString& label, const QString& value) {
                QLineEdit* field = new QLineEdit(tab);
                field->setReadOnly(true);
                field->setFocusPolicy(Qt::NoFocus);
                field->setText(value);
                layout->addRow(label + ":", field);
            };

            addReadOnlyField("Interface Name", QString::fromStdString(net.GetName()));
            addReadOnlyField("IPv4", QString::fromStdString(net.GetIpv4()));
            addReadOnlyField("IPv6", QString::fromStdString(net.GetIpv6()));
            addReadOnlyField("IPv4 Netmask", QString::fromStdString(net.GetIpv4Netmask()));
            addReadOnlyField("IPv6 Netmask", QString::fromStdString(net.GetIpv6Netmask()));
            addReadOnlyField("MAC Address", QString::fromStdString(net.GetMac()));
            addReadOnlyField("Gateway", QString::fromStdString(net.GatGateway()));

            networkInnerTabWidget->addTab(tab, QString("Interface %1").arg(i+1));
        }
    }

    // Вкладка DNS
    QWidget* dnsTab = new QWidget();
    QFormLayout* dnsLayout = new QFormLayout(dnsTab);
    QListWidget* dnsLabel = new QListWidget(dnsTab);
    dnsLabel->setFocusPolicy(Qt::NoFocus);
    dnsLayout->addRow("DNS Servers:", dnsLabel);

    auto dnsList = systemMonitor.GetDNS();
    if (dnsList.empty()) {
        dnsLabel->addItem("-");
    } else {
        std::set<std::string> uniqueDns;
        for (const auto& dns : dnsList) {
            uniqueDns.insert(dns.empty() ? "-" : dns);
        }
        for (const auto& dns : uniqueDns) {
            dnsLabel->addItem(QString::fromStdString(dns));
        }
    }

    networkInnerTabWidget->addTab(dnsTab, "DNS");
}

void MainWindow::updateAboutTab()
{
    ui->versionLabel->setText("1.0");
    ui->authorLabel->setText("System Monitor");
}

MainWindow::~MainWindow()
{
    delete ui;
}
