#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

namespace Devices {
struct Uptime {
  int days = 0;
  int hours = 0;
  int minutes = 0;
};

class PC;
class Device {
protected:
  std::string name;

public:
  Device();
  Device(std::string name);
  Device(const Device &other);
  Device &operator=(const Device &other);
  std::string GetName() const;
  friend class PC;
};

class CPU : public Device {
private:
  size_t cores;
  size_t threats;
  std::string maxSpeed;
  std::string socket;
  std::string l1Cache;
  std::string l2Cache;
  std::string l3Cache;
  int temperature;

public:
  CPU();
  CPU(const CPU &other);

  size_t GetCores() const;
  size_t GetThreats() const;
  std::string GetMaxSpeed() const;
  std::string GetSocket() const;
  std::string GetL1Cache() const;
  std::string GetL2Cache() const;
  std::string GetL3Cache() const;
  int GetTemperature() const;
  friend class PC;
};

class RAM : public Device {
private:
  std::string size;
  std::string formFactor;
  std::string type;
  std::string manufacturer;
  std::string speed;
  std::string channel;
  int rank;

public:
  RAM();
  RAM(const RAM &other);

  std::string GetSize() const;
  std::string GetFormFactor() const;
  std::string GetType() const;
  std::string GetManufacturer() const;
  std::string GetSpeed() const;
  std::string GetChannel() const;
  int GetRank() const;

  friend class PC;
};

class NetworkInterface : public Device {
private:
  std::string ipv4;
  std::string ipv6;
  std::string ipv4Netmask;
  std::string ipv6Netmask;
  std::string mac;
  std::string gateway;

public:
  NetworkInterface();
  NetworkInterface(const NetworkInterface &other);

  std::string GetIpv4() const;
  std::string GetIpv6() const;
  std::string GetIpv4Netmask() const;
  std::string GetIpv6Netmask() const;
  std::string GetMac() const;
  std::string GatGateway() const;
  friend class PC;
};

class PC {
private:
  PC();

  std::string hostname;
  Uptime uptime;

  std::vector<CPU> mainProcessors;
  static long long currentCPUUseIdle;
  static long long currentCPUUseTotal;
  double totalCPUUse;

  std::vector<RAM> RAMDevices;
  int summaryRAMVolume;
  int usedRAMVolume;

  std::vector<NetworkInterface> NIs;
  std::vector<std::string> DNS;

  std::vector<std::string> GPU;
  std::vector<std::string> NIControllers;

  void CollectHostname();
  void CollectStaticCPUData();
  void CollectStaticRAMData();
  void CollectPCIDevices();

  void CollectUptime();
  void CollectDynamicCPUData();
  void CollectDynamicRAMData();
  void CollectCommonNIsData();

public:
  PC(const PC &) = delete;
  PC &operator=(const PC &) = delete;
  static PC &GetInstance() {
    static PC currentPC;
    return currentPC;
  }

  void UpdateData();

  std::string GetHostname() const;
  struct Uptime GetUptime() const;
  std::vector<CPU> &GetCPU();
  double GetCPUUse() const;
  std::vector<RAM> &GetRam();
  int GetRAMVolume() const;
  int GetUsedRAMVolume() const;
  std::vector<NetworkInterface> &GetNIs();
  std::vector<std::string> &GetDNS();
  std::vector<std::string> &GetGPU();
  std::vector<std::string> &GetNIControllers();
};
} // namespace Devices