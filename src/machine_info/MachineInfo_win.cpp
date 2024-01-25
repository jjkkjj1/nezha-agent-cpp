#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "MachineInfo.h"
#include "nezha_util.h"
#include <dxgi.h>
#include <iostream>
#include <Pdh.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <chrono>
#include <iostream>
#include <regex>
#include <vector>
#include <TlHelp32.h>
#include <Pdh.h>
#include <PdhMsg.h>
#include <strsafe.h>
#include <tchar.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")



namespace nezha{
    std::shared_ptr<spdlog::logger> MachineInfo::logger;
    void MachineInfo::setLogger(const std::string &logger_name) {
        logger = spdlog::get(logger_name);
    }

    std::string MachineInfo::getPlatform() {
        auto platform = Util::readRegedit<std::string, wchar_t>(L"ProductName",
                                                              L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                                              REG_SZ);
        if(platform.starts_with("Windows 10")){
            auto currentBuildNumberStr = Util::readRegedit<std::string, wchar_t>(L"currentBuildNumber",
                                                                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                                                        REG_SZ);
            auto currentBuildNumber = std::stoi(currentBuildNumberStr);
            if(currentBuildNumber>=22000){
                platform = platform.replace(0, 10, "Windows 11");
            }
        }
        LOG_DEBUG("platform: {}", platform);
        return platform;
    }

    std::string MachineInfo::getPlatformVersion() {
        DWORD dwMajorVersion = 0;
        DWORD dwMinorVersion = 0;
        DWORD dwBuildNumber = 0;
        LONG(WINAPI* RtlGetVersion)(LPOSVERSIONINFOEXW);

        OSVERSIONINFOEXW osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

        HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
        if (hNtDll)
        {
            RtlGetVersion = reinterpret_cast<LONG(WINAPI*)(LPOSVERSIONINFOEXW)>(GetProcAddress(hNtDll, "RtlGetVersion"));
            if (RtlGetVersion)
            {
                if (RtlGetVersion(&osvi) == 0)
                {
                    dwMajorVersion = osvi.dwMajorVersion;
                    dwMinorVersion = osvi.dwMinorVersion;
                    dwBuildNumber = osvi.dwBuildNumber;

                    auto urb = Util::readRegedit<DWORD, DWORD>(L"UBR",
                                                                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                                                            REG_DWORD   );
                    auto displayVersion = Util::readRegedit<std::string, wchar_t>(L"DisplayVersion",
                                                               L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                                                               REG_SZ);
                    auto platformVersion = std::vformat("{}.{}.{}.{}_{}_Build {}.{}",
                                                        std::make_format_args(dwMajorVersion,dwMinorVersion,dwBuildNumber,urb,displayVersion,dwBuildNumber,urb));
                    LOG_DEBUG("Platform Version: {}", platformVersion);
                    return platformVersion;
                }
            }
        }
        LOG_ERROR("Failed to retrieve system version information.");
        return "";
    }

    std::string MachineInfo::getCPU() {
        int cpuInfo[4] = { 0 };
        char cpuName[MAX_PATH] = { 0 };

        // 获取 CPU 名称
        __cpuid(cpuInfo, 0x80000000);
        unsigned int extIds = cpuInfo[0];

        if (extIds >= 0x80000004)
        {
            __cpuidex(cpuInfo, 0x80000002, 0);
            std::memcpy(cpuName, cpuInfo, sizeof(cpuInfo));

            __cpuidex(cpuInfo, 0x80000003, 0);
            std::memcpy(cpuName + 16, cpuInfo, sizeof(cpuInfo));

            __cpuidex(cpuInfo, 0x80000004, 0);
            std::memcpy(cpuName + 32, cpuInfo, sizeof(cpuInfo));
        }
        else
        {
            // 如果不能获取到 CPU 名称，则使用默认的字符串
            std::memcpy(cpuName, "Unknown CPU", 11);
        }


        // 获取 CPU 数量
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        int processorCount = sysInfo.dwNumberOfProcessors;

        auto cpu_str = std::vformat("{} {} Physical Core",
                                    std::make_format_args(cpuName, processorCount));
        LOG_DEBUG("CPU: {}", cpu_str);
        return cpu_str;
    }

    unsigned long long MachineInfo::getTotalMemory() {
        MEMORYSTATUSEX memoryStatus;
        memoryStatus.dwLength = sizeof(memoryStatus);
        DWORDLONG totalMemory;
        if (GlobalMemoryStatusEx(&memoryStatus))
        {
            totalMemory = memoryStatus.ullTotalPhys;
            LOG_DEBUG("Total Memory: {} MB", totalMemory / (1024 * 1024));
        }
        return totalMemory;
    }

    unsigned long long MachineInfo::getTotalDisk() {
        unsigned long long all_Free = 0;
        unsigned long long all_Total = 0;
        unsigned long long used= 0;
        DWORD dwSize = MAX_PATH;
        TCHAR szLogicalDrives[MAX_PATH] = { 0 };

        DWORD dwResult = GetLogicalDriveStrings(dwSize, szLogicalDrives);

        if (dwResult > 0 && dwResult <= MAX_PATH)
        {
            TCHAR* szSingleDrive = szLogicalDrives;

            while (*szSingleDrive)
            {
                uint64_t available, total, free;
                if (GetDiskFreeSpaceEx(szSingleDrive, (ULARGE_INTEGER*)&available, (ULARGE_INTEGER*)&total, (ULARGE_INTEGER*)&free))
                {
                    uint64_t Total, Available, Free;
                    Total = total;
                    Available = available;
                    Free = free;

                    all_Total += Total;   //总
                    all_Free += Free;   //剩余
                }
                // 获取下一个驱动器号起始地址
                szSingleDrive += wcslen(szSingleDrive) + 1;
            }
        }
        LOG_DEBUG("Total Disk: {} GB", all_Total >> 30);
        return all_Total;
    }

    unsigned long long MachineInfo::getTotalSwap() {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            LOG_DEBUG("Total Virtual Memory: {} GB", memInfo.ullTotalPageFile >> 30);
            return memInfo.ullTotalPageFile;
        }
        return 0;
    }

    std::string MachineInfo::getArchitecture() {
        std::string architecture;
#if defined(_M_X64)
        architecture = "x86_64";
#elif defined(_M_IX86)
        architecture = "x86";
    #elif defined(_M_IA64)
        architecture = "IA64";
    #elif defined(_M_ARM)
        architecture = "ARM";
    #else
        architecture = "Unknown";
#endif
        LOG_DEBUG("Architecture: {}", architecture);
        return architecture;
    }

    unsigned long long MachineInfo::getMachineLaunchTime() {
        ULONGLONG tickCount = GetTickCount64();
        ULONGLONG bootTime = time(nullptr) - (tickCount / 1000);
        LOG_DEBUG("Boot Time: {}", bootTime);
        return bootTime;
    }

    struct GPUInfo {
        std::string name;
        unsigned long long memory;
    };

    std::vector<MachineInfo::GPUInfo> MachineInfo::getGPUs() {
        std::vector<GPUInfo> gpus;

        // 参数定义
        IDXGIFactory * pFactory;
        IDXGIAdapter * pAdapter;
        std::vector <IDXGIAdapter*> vAdapters;            // 显卡

        // 显卡的数量
        int iAdapterNum = 0;

        // 创建一个DXGI工厂
        HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));

        if (FAILED(hr))
            return gpus;

        // 枚举适配器
        while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            vAdapters.push_back(pAdapter);
            ++iAdapterNum;
        }

        for (size_t i = 0; i < vAdapters.size(); i++)
        {
            // 获取信息
            DXGI_ADAPTER_DESC adapterDesc;
            vAdapters[i]->GetDesc(&adapterDesc);
            // 输出显卡信息
            auto gpuName=Util::utf16ToUtf8(adapterDesc.Description);
            auto gpuMem = (double)adapterDesc.DedicatedVideoMemory /1024/1024/1024;
            auto gpu = GPUInfo{gpuName, gpuMem};
            if(std::find(gpus.begin(), gpus.end(), gpu)!=gpus.end()){
                continue;
            }
            gpus.push_back({gpuName, gpuMem});
            LOG_DEBUG("GPU: {} {} GB", gpuName, gpuMem);
        }
        vAdapters.clear();
        return gpus;
    }

    bool init_cpu = false;
    std::mutex cpu_usage_mtx;
    double MachineInfo::getCPUUsage() {
        static PDH_HQUERY cpuQuery;
        static PDH_HCOUNTER cpuTotal;
        std::lock_guard<std::mutex> lock(cpu_usage_mtx);
        if(!init_cpu){
            init_cpu = true;
            PdhOpenQuery(nullptr, NULL, &cpuQuery);
            PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
            PdhCollectQueryData(cpuQuery);
        }

        PDH_FMT_COUNTERVALUE counterVal;
        PdhCollectQueryData(cpuQuery);
        PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, nullptr, &counterVal);
        LOG_DEBUG("CPU Usage: {}%", counterVal.doubleValue);
        return counterVal.doubleValue;
    }

    unsigned long long MachineInfo::getMemUsage() {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            unsigned long long totalPhysicalMemory = memInfo.ullTotalPhys;
            unsigned long long availPhysicalMemory = memInfo.ullAvailPhys;
            auto ret = totalPhysicalMemory - availPhysicalMemory;
            LOG_DEBUG("Memory Usage: {} MB", (ret) / (1024 * 1024));
            return ret;
        }
        return 0;
    }

    unsigned long long MachineInfo::getSwapUsage() {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            unsigned long long totalVirtualMemory = memInfo.ullTotalPageFile;
            unsigned long long availVirtualMemory = memInfo.ullAvailPageFile;
            auto ret = totalVirtualMemory - availVirtualMemory;
            LOG_DEBUG("Virtual Memory Usage: {} MB", (ret) / (1024 * 1024));
            return ret;
        }
        return 0;
    }

    unsigned long long MachineInfo::getDiskUsage() {
        unsigned long long all_Free = 0;
        unsigned long long all_Total = 0;
        unsigned long long used= 0;
        DWORD dwSize = MAX_PATH;
        TCHAR szLogicalDrives[MAX_PATH] = { 0 };

        DWORD dwResult = GetLogicalDriveStrings(dwSize, szLogicalDrives);

        if (dwResult > 0 && dwResult <= MAX_PATH)
        {
            TCHAR* szSingleDrive = szLogicalDrives;

            while (*szSingleDrive)
            {
                uint64_t available, total, free;
                if (GetDiskFreeSpaceEx(szSingleDrive, (ULARGE_INTEGER*)&available, (ULARGE_INTEGER*)&total, (ULARGE_INTEGER*)&free))
                {
                    uint64_t Total, Available, Free;
                    Total = total;
                    Available = available;
                    Free = free;

                    all_Total += Total;   //总
                    all_Free += Free;   //剩余
                }
                // 获取下一个驱动器号起始地址
                szSingleDrive += wcslen(szSingleDrive) + 1;
            }
        }
        auto ret = all_Total - all_Free;
        LOG_DEBUG("Disk Usage: {} GB", ret >> 30);
        return ret;
    }

    std::tuple<unsigned long long, unsigned long long> MachineInfo::getNetTransfer() {
        unsigned long long totalIn = 0;
        unsigned long long totalOut = 0;
        std::vector<std::string> address;
        PIP_ADAPTER_INFO adapterInfo = nullptr;
        ULONG bufferSize = 0;
        if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
            adapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(new char[bufferSize]);
            if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
                PIP_ADAPTER_INFO adapter = adapterInfo;
                while (adapter != nullptr) {
                    address.emplace_back(adapter->AdapterName);
                    MIB_IF_ROW2 ifRow = {};
                    ifRow.InterfaceIndex = adapter->Index;
                    if ( GetIfEntry2(&ifRow) != NO_ERROR) {
                        LOG_ERROR("Failed to get interface info");
                    }else{
                        totalOut += ifRow.OutOctets;
                        totalIn += ifRow.InOctets;
                    }
                    adapter = adapter->Next;
                }
            } else {
                LOG_ERROR("Failed to get adapter info");
            }
            delete[] reinterpret_cast<char*>(adapterInfo);
        } else {
            LOG_ERROR("Failed to get adapter info buffer size");
        }

        LOG_DEBUG("Net In Transfer: {} MB", totalIn / (1024 * 1024));
        LOG_DEBUG("Net Out Transfer: {} MB", totalOut / (1024 * 1024));
        return {totalIn, totalOut};
    }

    bool init_speed = false;
    std::tuple<unsigned long long, unsigned long long> MachineInfo::getNetSpeed() {
        static std::tuple<unsigned long long, unsigned long long> last_transfer;
        static time_t last_time;
        if(!init_speed){
            init_speed = true;
            last_transfer = getNetTransfer();
            last_time = std::time(nullptr);
            return {0,0};
        }
        auto cur_transfer = getNetTransfer();
        auto cur_time = std::time(nullptr);
        auto time_diff = cur_time - last_time;
        auto in_diff = std::get<0>(cur_transfer) - std::get<0>(last_transfer);
        auto out_diff = std::get<1>(cur_transfer) - std::get<1>(last_transfer);
        auto in_speed = in_diff / time_diff;
        auto out_speed = out_diff / time_diff;
        LOG_DEBUG("Net In Speed: {} KB/s", in_speed / 1024);
        LOG_DEBUG("Net Out Speed: {} KB/s", out_speed / 1024);
        last_transfer = cur_transfer;
        last_time = cur_time;
        return {in_speed, out_speed};
    }

    double samplingFrequency = 5; // in seconds
    double loadAvgFactor1M = 1 / std::exp(samplingFrequency / 60);
    double loadAvgFactor5M = 1 / std::exp(samplingFrequency / (5 * 60));
    double loadAvgFactor15M = 1 / std::exp(samplingFrequency / (15 * 60));
    double currentLoad = 0;

    std::mutex loadAvgMutex;
    double loadAvg1M = 0;
    double loadAvg5M = 0;
    double loadAvg15M = 0;
    // This function should return the current load
    bool init_load_ = false;
    double getLoad() {
        static PDH_HQUERY loadQuery;
        static PDH_HCOUNTER loadTotal;
        if(!init_load_){
            init_load_ = true;
            PdhOpenQuery(nullptr, NULL, &loadQuery);
            PdhAddEnglishCounterW(loadQuery, L"\\System\\Processor Queue Length", NULL, &loadTotal);
            PdhCollectQueryData(loadQuery);
            return 0;
        }

        PDH_FMT_COUNTERVALUE counterVal;
        PdhCollectQueryData(loadQuery);
        PdhGetFormattedCounterValue(loadTotal, PDH_FMT_DOUBLE, nullptr, &counterVal);
        return counterVal.doubleValue;
    }

    void updateLoadAvg() {
        currentLoad = getLoad();
        std::lock_guard<std::mutex> lock(loadAvgMutex);
        loadAvg1M = loadAvg1M * loadAvgFactor1M + currentLoad * (1 - loadAvgFactor1M);
        loadAvg5M = loadAvg5M * loadAvgFactor5M + currentLoad * (1 - loadAvgFactor5M);
        loadAvg15M = loadAvg15M * loadAvgFactor15M + currentLoad * (1 - loadAvgFactor15M);
    }
    bool init_load = false;
    std::tuple<double, double, double> MachineInfo::getLoadAverage() {
        if(!init_load){
            init_load = true;
            updateLoadAvg();
            std::thread ([]() {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                    updateLoadAvg();
                }
            }).detach();
            return {0, 0, 0};
        }
        std::lock_guard<std::mutex> lock(loadAvgMutex);
        LOG_DEBUG("Load Average 1M: {}", loadAvg1M);
        LOG_DEBUG("Load Average 5M: {}", loadAvg5M);
        LOG_DEBUG("Load Average 15M: {}", loadAvg15M);
        return {loadAvg1M, loadAvg5M, loadAvg15M};
    }

    std::vector<std::pair<int, int>> GetGPURunningTimeProcess() {
        std::vector<std::pair<int, int>> ret;

        DWORD counterListSize = 0;
        DWORD instanceListSize = 0;
        DWORD dwFlags = 0;
        const auto COUNTER_OBJECT = TEXT("GPU Engine");
        PDH_STATUS status = ERROR_SUCCESS;
        status = PdhEnumObjectItems(nullptr, nullptr, COUNTER_OBJECT, nullptr,
                                    &counterListSize, nullptr, &instanceListSize,
                                    PERF_DETAIL_WIZARD, dwFlags);
        if (status != PDH_MORE_DATA) {
            throw std::runtime_error("failed PdhEnumObjectItems()");
        }

        std::vector<TCHAR> counterList(counterListSize);
        std::vector<TCHAR> instanceList(instanceListSize);
        status = ::PdhEnumObjectItems(
                nullptr, nullptr, COUNTER_OBJECT, counterList.data(), &counterListSize,
                instanceList.data(), &instanceListSize, PERF_DETAIL_WIZARD, dwFlags);
        if (status != ERROR_SUCCESS) {
            throw std::runtime_error("failed PdhEnumObjectItems()");
        }

        for (TCHAR* pTemp = instanceList.data(); *pTemp != 0;
             pTemp += _tcslen(pTemp) + 1) {
            if (::_tcsstr(pTemp, TEXT("engtype_3D")) == NULL) {
                continue;
            }

            TCHAR buffer[1024];
            ::StringCchCopy(buffer, 1024, TEXT("\\GPU Engine("));
            ::StringCchCat(buffer, 1024, pTemp);
            ::StringCchCat(buffer, 1024, TEXT(")\\Running time"));

            HQUERY hQuery = NULL;
            status = ::PdhOpenQuery(NULL, 0, &hQuery);
            if (status != ERROR_SUCCESS) {
                continue;
            }

            HCOUNTER hCounter = NULL;
            status = ::PdhAddCounter(hQuery, buffer, 0, &hCounter);
            if (status != ERROR_SUCCESS) {
                continue;
            }

            status = ::PdhCollectQueryData(hQuery);
            if (status != ERROR_SUCCESS) {
                continue;
            }

            status = ::PdhCollectQueryData(hQuery);
            if (status != ERROR_SUCCESS) {
                continue;
            }

            const DWORD dwFormat = PDH_FMT_LONG;
            PDH_FMT_COUNTERVALUE ItemBuffer;
            status =
                    ::PdhGetFormattedCounterValue(hCounter, dwFormat, nullptr, &ItemBuffer);
            if (ERROR_SUCCESS != status) {
                continue;
            }

            if (ItemBuffer.longValue > 0) {
#ifdef _UNICODE
                std::wregex re(TEXT("pid_(\\d+)"));
                std::wsmatch sm;
                std::wstring str = pTemp;
#else
                std::regex re(TEXT("pid_(\\d+)"));
      std::smatch sm;
      std::string str = pTemp;
#endif
                if (std::regex_search(str, sm, re)) {
                    int pid = std::stoi(sm[1]);
                    ret.push_back({pid, ItemBuffer.longValue});
                }
            }

            ::PdhCloseQuery(hQuery);
        }

        return ret;
    }

    int64_t GetGPURunningTimeTotal() {
        int64_t total = 0;
        std::vector<std::pair<int, int>> list = GetGPURunningTimeProcess();
        for (const std::pair<int, int>& v : list) {
            if (v.second > 0) {
                total += v.second;
            }
        }
        return total;
    }

    double GetGPUUsage() {
        static std::chrono::steady_clock::time_point prev_called =
                std::chrono::steady_clock::now();
        static int64_t prev_running_time = GetGPURunningTimeTotal();

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration elapsed = now - prev_called;

        int64_t elapsed_sec =
                std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
        int64_t running_time = GetGPURunningTimeTotal();

        double percentage =
                (double)(running_time - prev_running_time) / elapsed_sec * 100;
        // printf("percent = (%lld - %lld) / %lld * 100 = %f\n", running_time,
        // prev_running_time, elapsed_sec, percentage);

        prev_called = now;
        prev_running_time = running_time;

        if (percentage > 1.0)
            percentage = 1.0;
        else if (percentage < 0.0)
            percentage = 0.0;
        return percentage;
    }

    double MachineInfo::getGPUUsage() {
        auto ret = GetGPUUsage();
        LOG_DEBUG("GPU Usage: {}%", ret);
        return ret;
    }

    unsigned long long MachineInfo::getTcpCount() {
        DWORD dwSize = 0;
        DWORD dwRetVal = 0;
        DWORD dwTcp6Count = 0;
        DWORD dwTcpCount = 0;

        // Call GetTcp6Table2 to get the necessary size into dwSize
        dwRetVal = GetTcp6Table2(NULL, &dwSize, FALSE);
        if (dwRetVal != ERROR_INSUFFICIENT_BUFFER) {
            LOG_ERROR("GetTcp6Table2 failed with error {}", dwRetVal);
            return 0;
        }

        // Allocate memory from size of TCP table
        PMIB_TCP6TABLE2 pTcp6Table = (MIB_TCP6TABLE2*)malloc(dwSize);
        if (pTcp6Table == NULL) {
            LOG_ERROR("Error allocating memory");
            return 0;
        }

        // Get the TCP6 table
        if ((dwRetVal = GetTcp6Table2(pTcp6Table, &dwSize, TRUE)) == NO_ERROR) {
            dwTcp6Count = pTcp6Table->dwNumEntries;
        } else {
            LOG_ERROR("GetTcp6Table2 failed with error {}", dwRetVal);
            free(pTcp6Table);
            return 0;
        }

        // Clean up
        free(pTcp6Table);

        // Reset dwSize for GetTcpTable2
        dwSize = 0;

        // Call GetTcpTable2 to get the necessary size into dwSize
        dwRetVal = GetTcpTable2(NULL, &dwSize, FALSE);
        if (dwRetVal != ERROR_INSUFFICIENT_BUFFER) {
            LOG_ERROR("GetTcpTable2 failed with error {}", dwRetVal);
            return 0;
        }

        // Allocate memory from size of TCP table
        PMIB_TCPTABLE2 pTcpTable = (MIB_TCPTABLE2*)malloc(dwSize);
        if (pTcpTable == NULL) {
            LOG_ERROR("Error allocating memory");
            return 0;
        }

        // Get the TCP table
        if ((dwRetVal = GetTcpTable2(pTcpTable, &dwSize, TRUE)) == NO_ERROR) {
            dwTcpCount = pTcpTable->dwNumEntries;
        } else {
            LOG_ERROR("GetTcpTable2 failed with error {}", dwRetVal);
            free(pTcpTable);
            return 0;
        }

        // Clean up
        free(pTcpTable);

        LOG_DEBUG("Number of TCP IPv4 connections: {}", dwTcpCount);
        LOG_DEBUG("Number of TCP IPv6 connections: {}", dwTcp6Count);
        auto total = dwTcp6Count + dwTcpCount;
        LOG_DEBUG("Total number of TCP connections: {}", total);
        return total;
    }

    unsigned long long MachineInfo::getUdpCount() {
        DWORD dwSize = 0;
        DWORD dwRetVal = 0;
        DWORD dwUdp6Count = 0;
        DWORD dwUdpCount = 0;

        // Call GetUdp6Table2 to get the necessary size into dwSize
        dwRetVal = GetUdp6Table(NULL, &dwSize, FALSE);
        if (dwRetVal != ERROR_INSUFFICIENT_BUFFER) {
            LOG_ERROR("GetUdp6Table failed with error {}", dwRetVal);
            return 0;
        }

        // Allocate memory from size of UDP table
        PMIB_UDP6TABLE pUdp6Table = (MIB_UDP6TABLE*)malloc(dwSize);
        if (pUdp6Table == NULL) {
            LOG_ERROR("Error allocating memory");
            return 0;
        }

        // Get the UDP6 table
        if ((dwRetVal = GetUdp6Table(pUdp6Table, &dwSize, TRUE)) == NO_ERROR) {
            dwUdp6Count = pUdp6Table->dwNumEntries;
        } else {
            LOG_ERROR("GetUdp6Table2 failed with error {}", dwRetVal);
            free(pUdp6Table);
            return 0;
        }

        // Clean up
        free(pUdp6Table);

        // Reset dwSize for GetUdpTable
        dwSize = 0;

        // Call GetUdpTable to get the necessary size into dwSize
        dwRetVal = GetUdpTable(NULL, &dwSize, FALSE);
        if (dwRetVal != ERROR_INSUFFICIENT_BUFFER) {
            LOG_ERROR("GetUdpTable failed with error {}", dwRetVal);
            return 0;
        }

        // Allocate memory from size of UDP table
        PMIB_UDPTABLE pUdpTable = (MIB_UDPTABLE*)malloc(dwSize);
        if (pUdpTable == NULL) {
            LOG_ERROR("Error allocating memory");
            return 0;
        }

        // Get the UDP table
        if ((dwRetVal = GetUdpTable(pUdpTable, &dwSize, TRUE)) == NO_ERROR) {
            dwUdpCount = pUdpTable->dwNumEntries;
        } else {
            LOG_ERROR("GetUdpTable failed with error {}", dwRetVal);
            free(pUdpTable);
            return 0;
        }

        // Clean up
        free(pUdpTable);

        LOG_DEBUG("Number of UDP IPv4 connections: {}", dwUdpCount);
        LOG_DEBUG("Number of UDP IPv6 connections: {}", dwUdp6Count);
        auto total = dwUdp6Count + dwUdpCount;
        LOG_DEBUG("Total number of UDP connections: {}", total);
        return total;
    }

    unsigned long long MachineInfo::getProcessCount() {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            LOG_ERROR("CreateToolhelp32Snapshot failed with error: {}", GetLastError());
            return 0;
        }

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hSnapshot, &pe)) {
            LOG_ERROR("Process32First failed with error: {}", GetLastError());
            CloseHandle(hSnapshot);
            return 0;
        }

        int processCount = 0;
        do {
            processCount++;
        } while (Process32Next(hSnapshot, &pe));

        CloseHandle(hSnapshot);
        LOG_DEBUG("Number of processes: {}", processCount);
        return processCount;
    }

    void MachineInfo::Init() {
        getCPUUsage();
        getLoad();
        getGPUUsage();
        getNetSpeed();
        Sleep(1000);
    }

}

