#include "SysMonCore.hpp"
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <fstream>
#include <ifaddrs.h>
#include <iostream>
#include <net/ethernet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sensors/sensors.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <thread>

namespace {
std::string lsCache(std::string cacheID) {
    FILE *pipe = popen("sudo dmidecode -t cache", "r");
    if (!pipe) {
        std::cout << "Failed to run dmidecode" << std::endl;
        return "";
    }
    std::string currentCache;
    char buffer[256];
    bool find = false;

    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        if (line.find(cacheID.substr(0, cacheID.size() - 1)) != std::string::npos) {
            find = true;
        } else if (line.find("Installed Size:") != std::string::npos && find) {
            currentCache = line.substr(line.find(":") + 2);
            break;
        }
    }

    pclose(pipe);
    currentCache.pop_back();
    return currentCache;
}
} // namespace

namespace Devices {
long long PC::currentCPUUseIdle = 0;
long long PC::currentCPUUseTotal = 0;

Device::Device() : name("-") {}
Device::Device(std::string name) : name(name) {}
Device::Device(const Device &other) : name(other.name) {}
Device &Device::operator=(const Device &other) {
    this->name = other.name;
    return *this;
}
std::string Device::GetName() const { return this->name; }

CPU::CPU()
    : Device("-"), cores(0), threats(0), maxSpeed("-"), socket("-"),
    l1Cache("-"), l2Cache("-"), l3Cache("-"), temperature(0) {}
CPU::CPU(const CPU &other)
    : Device(other.name), cores(other.cores), threats(other.threats),
    maxSpeed(other.maxSpeed), socket(other.socket), l1Cache(other.l1Cache),
    l2Cache(other.l2Cache), l3Cache(other.l3Cache),
    temperature(other.temperature) {}

size_t CPU::GetCores() const { return this->cores; }
size_t CPU::GetThreats() const { return this->threats; }
std::string CPU::GetMaxSpeed() const { return this->maxSpeed; }
std::string CPU::GetSocket() const { return this->socket; }
std::string CPU::GetL1Cache() const { return this->l1Cache; }
std::string CPU::GetL2Cache() const { return this->l2Cache; }
std::string CPU::GetL3Cache() const { return this->l3Cache; }
int CPU::GetTemperature() const { return this->temperature; }

RAM::RAM()
    : Device("-"), size("-"), formFactor("-"), type("-"), manufacturer("-"),
    speed("-"), channel("-"), rank(0) {}
RAM::RAM(const RAM &other)
    : Device(other.name), size(other.size), formFactor(other.formFactor),
    type(other.type), manufacturer(other.manufacturer), speed(other.speed),
    channel(other.channel), rank(other.rank) {}

std::string RAM::GetSize() const { return this->size; }
std::string RAM::GetFormFactor() const { return this->formFactor; }
std::string RAM::GetType() const {
    return this->type;
    ;
}
std::string RAM::GetManufacturer() const { return this->manufacturer; }
std::string RAM::GetSpeed() const { return this->speed; }
std::string RAM::GetChannel() const { return this->channel; }
int RAM::GetRank() const {
    return this->rank;
    ;
}

void PC::CollectHostname() {
    std::ifstream hostnameFile("/proc/sys/kernel/hostname");
    if (hostnameFile.is_open()) {
        getline(hostnameFile, hostname);
    } else {
        hostname = "-";
    }
}

void PC::CollectStaticCPUData() {
    FILE *pipe = popen("sudo dmidecode -t processor", "r");
    if (pipe) {
        CPU current_proc;
        char buffer[256];

        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);

            if (line.find("Socket Designation:") != std::string::npos) {
                current_proc.socket = line.substr(line.find(":") + 2);
                current_proc.socket.pop_back();
            } else if (line.find("Max Speed:") != std::string::npos) {
                current_proc.maxSpeed = line.substr(line.find(":") + 2);
                current_proc.maxSpeed.pop_back();
            } else if (line.find("Version:") != std::string::npos) {
                current_proc.name = line.substr(line.find(":") + 2);
                current_proc.name.pop_back();
            } else if (line.find("Core Count:") != std::string::npos) {
                std::stringstream ss(line.substr(line.find(":") + 2));
                ss >> current_proc.cores;
            } else if (line.find("Thread Count:") != std::string::npos) {
                std::stringstream ss(line.substr(line.find(":") + 2));
                ss >> current_proc.threats;
            } else if (line.find("L1 Cache Handle:") != std::string::npos) {
                current_proc.l1Cache = lsCache(line.substr(line.find(":") + 2));
            } else if (line.find("L2 Cache Handle:") != std::string::npos) {
                current_proc.l2Cache = lsCache(line.substr(line.find(":") + 2));
            } else if (line.find("L3 Cache Handle:") != std::string::npos) {
                current_proc.l3Cache = lsCache(line.substr(line.find(":") + 2));
            } else if (line.find("Processor Information") != std::string::npos &&
                       current_proc.cores != 0) {
                mainProcessors.push_back(current_proc);
                current_proc.cores = 0;
            }
        }
        mainProcessors.push_back(current_proc);
        pclose(pipe);
    }
}

void PC::CollectStaticRAMData() {
    FILE *pipe = popen("sudo dmidecode -t memory", "r");
    if (pipe) {
        RAM currentRAM;
        char buffer[256];

        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);

            if (line.find("\tSize:") != std::string::npos) {
                currentRAM.size = line.substr(line.find(":") + 2);
                currentRAM.size.pop_back();
            } else if (line.find("Form Factor:") != std::string::npos) {
                currentRAM.formFactor = line.substr(line.find(":") + 2);
                currentRAM.formFactor.pop_back();
            } else if (line.find("Bank Locator:") != std::string::npos) {
                currentRAM.channel = line.substr(line.find(":") + 2);
                currentRAM.channel.pop_back();
            } else if (line.find("Type:") != std::string::npos) {
                currentRAM.type = line.substr(line.find(":") + 2);
                currentRAM.type.pop_back();
            } else if (line.find("Manufacturer:") != std::string::npos) {
                currentRAM.manufacturer = line.substr(line.find(":") + 2);
                currentRAM.manufacturer.pop_back();
            } else if (line.find("Part Number:") != std::string::npos) {
                currentRAM.name = line.substr(line.find(":") + 2);
                currentRAM.name.pop_back();
            } else if (line.find("Configured Memory Speed:") != std::string::npos) {
                currentRAM.speed = line.substr(line.find(":") + 2);
                currentRAM.speed.pop_back();
            } else if (line.find("Rank:") != std::string::npos) {
                std::stringstream temp(line.substr(line.find(":") + 2));
                temp >> currentRAM.rank;
            } else if (line.find("Memory Device") != std::string::npos &&
                       currentRAM.rank != 0) {
                if (currentRAM.channel.empty()) {
                    currentRAM.channel = "Single";
                }
                RAMDevices.push_back(currentRAM);
                currentRAM.rank = 0;
            }
        }
        if (currentRAM.channel.empty()) {
            currentRAM.channel = "Single";
        }
        RAMDevices.push_back(currentRAM);
        pclose(pipe);
    }
}

NetworkInterface::NetworkInterface()
    : Device("-"), ipv4("-"), ipv6("-"), ipv4Netmask("-"), ipv6Netmask("-"),
    mac("-"), gateway("-") {};
NetworkInterface::NetworkInterface(const NetworkInterface &other)
    : Device(other.name), ipv4(other.ipv4), ipv6(other.ipv6),
    ipv4Netmask(other.ipv4Netmask), ipv6Netmask(other.ipv4Netmask),
    mac(other.mac), gateway(other.gateway) {}

std::string NetworkInterface::GetIpv4() const { return this->ipv4; }
std::string NetworkInterface::GetIpv6() const { return this->ipv6; }
std::string NetworkInterface::GetIpv4Netmask() const {
    return this->ipv4Netmask;
}
std::string NetworkInterface::GetIpv6Netmask() const {
    return this->ipv6Netmask;
}

std::string NetworkInterface::GetMac() const { return this->mac; }

std::string NetworkInterface::GatGateway() const { return this->gateway; }

void PC::CollectUptime() {
    std::ifstream uptimeFile("/proc/uptime");
    if (uptimeFile.is_open()) {
        double uptimeInSeconds{};
        uptimeFile >> uptimeInSeconds;

        uptime.days = uptimeInSeconds / (24 * 3600);
        uptime.hours = (uptimeInSeconds - uptime.days * 24 * 3600) / 3600;
        uptime.minutes =
            (uptimeInSeconds - uptime.days * 24 * 3600 - uptime.hours * 3600) /
            60;

        uptimeFile.close();
    }
}

void PC::CollectDynamicCPUData() {
    for (int i = 0; i < 2; ++i) {
        std::ifstream cpuUsageFile("/proc/stat");
        std::string line;

        if (std::getline(cpuUsageFile, line)) {
            if (line.compare(0, 3, "cpu") == 0) {
                std::istringstream iss(line);
                std::string cpuName;
                long long userProcess = 0;
                long long niceProcess = 0;
                long long system = 0;
                long long idle = 0;
                long long iowait = 0;
                long long irq = 0;
                long long softirq = 0;
                long long steal = 0;

                iss >> cpuName >> userProcess >> niceProcess >> system >> idle >>
                    iowait >> irq >> softirq >> steal;

                long long totalIdle = idle + iowait;
                long long totalNotIdle =
                    userProcess + niceProcess + system + irq + softirq + steal;
                long long total = totalIdle + totalNotIdle;

                if (currentCPUUseTotal != 0) {
                    double differenceTotal = total - currentCPUUseTotal;
                    double differenceIdle = totalIdle - currentCPUUseIdle;
                    totalCPUUse =
                        (differenceTotal - differenceIdle) / differenceTotal * 100.0;

                    currentCPUUseTotal = total;
                    currentCPUUseIdle = totalIdle;

                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }

                currentCPUUseTotal = total;
                currentCPUUseIdle = totalIdle;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::vector<CPU>::iterator temp = mainProcessors.begin();
    sensors_init(nullptr);

    sensors_chip_name const *chip;
    int chip_nr = 0;

    while ((chip = sensors_get_detected_chips(nullptr, &chip_nr))) {
        if (chip->prefix &&
            (strstr(chip->prefix, "coretemp") || strstr(chip->prefix, "k10temp") ||
             strstr(chip->prefix, "zenpower"))) {

            sensors_feature const *feature;
            int feature_nr = 0;

            while ((feature = sensors_get_features(chip, &feature_nr))) {
                if (feature->type == SENSORS_FEATURE_TEMP) {
                    sensors_subfeature const *sub = sensors_get_subfeature(
                        chip, feature, SENSORS_SUBFEATURE_TEMP_INPUT);

                    if (sub) {
                        double val;
                        int rc = sensors_get_value(chip, sub->number, &val);
                        if (rc >= 0) {
                            temp->temperature = val;
                            ++temp;
                        }
                    }
                }
            }
        }
    }
    sensors_cleanup();
}

void PC::CollectDynamicRAMData() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        summaryRAMVolume =
            static_cast<double>(info.totalram * info.mem_unit) / (1024 * 1024);
        usedRAMVolume =
            static_cast<double>((info.totalram - info.freeram) * info.mem_unit) /
            (1024 * 1024);
    }
}

void PC::CollectCommonNIsData() {
    struct ifaddrs *ifaddr, *ifa;
    int family{};

    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        bool ifaceExists = false;
        NetworkInterface tempNIs{};
        tempNIs.name = ifa->ifa_name;

        if (ifa->ifa_addr == nullptr) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            char ip[INET_ADDRSTRLEN]{};
            char netmask[INET_ADDRSTRLEN]{};
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            struct sockaddr_in *mask = (struct sockaddr_in *)ifa->ifa_netmask;
            inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &mask->sin_addr, netmask, INET_ADDRSTRLEN);

            for (std::vector<NetworkInterface>::iterator temp = NIs.begin();
                 temp != NIs.end(); ++temp) {
                if (temp->name == ifa->ifa_name) {
                    ifaceExists = true;
                    temp->ipv4 = ip;
                    temp->ipv4Netmask = netmask;
                }
            }
            if (!ifaceExists) {
                tempNIs.ipv4 = ip;
                tempNIs.ipv4Netmask = netmask;
                NIs.push_back(tempNIs);
                ifaceExists = true;
            }
        }

        else if (family == AF_INET6) {
            char ip6[INET6_ADDRSTRLEN]{};
            char netmask6[INET6_ADDRSTRLEN]{};
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            struct sockaddr_in6 *mask6 = (struct sockaddr_in6 *)ifa->ifa_netmask;
            inet_ntop(AF_INET6, &addr6->sin6_addr, ip6, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &mask6->sin6_addr, netmask6, INET6_ADDRSTRLEN);

            for (std::vector<NetworkInterface>::iterator temp = NIs.begin();
                 temp != NIs.end(); ++temp) {
                if (temp->name == ifa->ifa_name) {
                    ifaceExists = true;
                    temp->ipv6 = ip6;
                    temp->ipv6Netmask = netmask6;
                }
            }
            if (!ifaceExists) {
                tempNIs.ipv6 = ip6;
                tempNIs.ipv6Netmask = netmask6;
                NIs.push_back(tempNIs);
                ifaceExists = true;
            }
        }

        if (!ifaceExists) {
            tempNIs.ipv4 = "-";
            tempNIs.ipv4Netmask = "-";
            tempNIs.ipv6 = "-";
            tempNIs.ipv6Netmask = "-";
            NIs.push_back(tempNIs);
        }
    }

    freeifaddrs(ifaddr);

    for (std::vector<NetworkInterface>::iterator temp = NIs.begin();
         temp != NIs.end(); ++temp) {
        std::string path = "/sys/class/net/" + temp->name + "/address";
        std::ifstream addressFile(path);
        if (addressFile) {
            std::getline(addressFile, temp->mac);

            addressFile.close();

            if (!temp->mac.empty() && temp->mac.back() == '\n') {
                temp->mac.pop_back();
            }
        } else {
            temp->mac = "-";
        }

        std::ifstream route("/proc/net/route");
        std::string line;

        while (std::getline(route, line)) {
            if (line.find(temp->name) != std::string::npos) {
                std::istringstream iss(line);
                std::string iface, dest, gateway;
                iss >> iface >> dest >> gateway;

                if (dest == "00000000") {
                    struct in_addr addr;
                    addr.s_addr = std::stoul(gateway, nullptr, 16);
                    temp->gateway = inet_ntoa(addr);
                }
            }
        }

        route.close();

        if (temp->gateway.empty()) {
            temp->gateway = "-";
        }
    }

    std::ifstream resolv("/etc/resolv.conf");
    std::string line;
    if (resolv) {
        while (std::getline(resolv, line)) {
            if(line.find("nameserver") != std::string::npos){
                int delimeterPos = line.find('%');
                if (delimeterPos == -1) {
                    delimeterPos = line.size();
                }
                DNS.push_back(
                    line.substr(11, line.size() - 11 - (line.size() - delimeterPos)));
            }
        }
        resolv.close();
    }
}

void PC::CollectPCIDevices() {
    FILE *pipe = popen("lspci", "r");
    if (pipe) {
        char buffer[256];

        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);

            if (line.find("VGA compatible controller:") != std::string::npos) {
                line = line.substr(line.find(":") + 2);
                line.pop_back();
                GPU.push_back(line.substr(line.find(":") + 2));
            } else if (line.find("Ethernet controller:") != std::string::npos ||
                       line.find("Network controller:") != std::string::npos) {
                line = line.substr(line.find(":") + 2);
                line.pop_back();
                NIControllers.push_back(line.substr(line.find(":") + 2));
            }
        }
        pclose(pipe);
    }
}

std::vector<std::string> &PC::GetGPU() { return this->GPU; }
std::vector<std::string> &PC::GetNIControllers() { return this->NIControllers; }

PC::PC() {
    CollectHostname();

    CollectStaticCPUData();

    CollectStaticRAMData();

    CollectPCIDevices();

    UpdateData();
}

void PC::UpdateData() {
    CollectUptime();

    CollectDynamicCPUData();

    CollectDynamicRAMData();

    CollectCommonNIsData();
}

std::string PC::GetHostname() const { return this->hostname; }
struct Uptime PC::GetUptime() const { return this->uptime; }
std::vector<CPU> &PC::GetCPU() { return this->mainProcessors; }
double PC::GetCPUUse() const { return this->totalCPUUse; }
std::vector<RAM> &PC::GetRam() { return this->RAMDevices; }
int PC::GetRAMVolume() const { return this->summaryRAMVolume; }
int PC::GetUsedRAMVolume() const { return this->usedRAMVolume; }
std::vector<NetworkInterface> &PC::GetNIs() { return this->NIs; }
std::vector<std::string> &PC::GetDNS() { return this->DNS; }
} // namespace Devices
