#ifndef NEZHA_AGENT_CPP_MACHINEINFO_H
#define NEZHA_AGENT_CPP_MACHINEINFO_H
#include <string>
#include <vector>
#include "spdlog/spdlog.h"

namespace nezha{
    class MachineInfo {
    public:
        struct GPUInfo {
            std::string name;
            double memory;
            bool operator == (const GPUInfo& other) const {
                return name == other.name && memory == other.memory;
            }
        };
    public:
        static void setLogger(const std::string& logger_name);
        static std::string getPlatform();
        static std::string getPlatformVersion();
        static std::string getCPU();
        static unsigned long long getTotalMemory();
        static unsigned long long getTotalDisk();
        static unsigned long long getTotalSwap();
        static std::string getArchitecture();
        static unsigned long long getMachineLaunchTime();
        static std::vector<GPUInfo> getGPUs();

        static double getCPUUsage();
        static double getGPUUsage();
        static unsigned long long getMemUsage();
        static unsigned long long getSwapUsage();
        static unsigned long long getDiskUsage();
        static std::tuple<unsigned long long, unsigned long long>
        getNetTransfer();
        static std::tuple<unsigned long long, unsigned long long>
        getNetSpeed();
        static std::tuple<double, double, double>
        getLoadAverage();
        static unsigned long long getTcpCount();
        static unsigned long long getUdpCount();
        static unsigned long long getProcessCount();

        static void Init();
    private:
        MachineInfo() = default;
        static std::shared_ptr<spdlog::logger> logger;
    };
}


#endif //NEZHA_AGENT_CPP_MACHINEINFO_H
