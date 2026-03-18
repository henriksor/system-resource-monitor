#include <iostream>
#include <windows.h>
#include <cstdint>

//RAM check - structure
struct  MemoryStatus {
        uint64_t totalBytes;
        uint64_t availableBytes;
    };

//CPU check -  structure
struct CpuTimes {
    uint64_t idle;
    uint64_t kernel;
    uint64_t user;
};

//RAM reading
MemoryStatus getRamStatus() {
    
    MemoryStatus status;
    
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

    status.totalBytes = statex.ullTotalPhys;
    status.availableBytes = statex.ullAvailPhys;

    return status;
}

// CPU monitor
struct CpuMonitor {
private:
    CpuTimes previous{};
    bool hasPrevious = false;

    CpuTimes getCurrentTimes() {
        FILETIME idleTime, kernelTime, userTime;

        GetSystemTimes(&idleTime, &kernelTime, &userTime);

        CpuTimes times;

        times.idle =
            (static_cast<uint64_t>(idleTime.dwHighDateTime) << 32) |
            idleTime.dwLowDateTime;

        times.kernel =
            (static_cast<uint64_t>(kernelTime.dwHighDateTime) << 32) |
            kernelTime.dwLowDateTime;

        times.user =
            (static_cast<uint64_t>(userTime.dwHighDateTime) << 32) |
            userTime.dwLowDateTime;

        return times;
    }

public:
    double getUsage() {
        CpuTimes current = getCurrentTimes();

        if (!hasPrevious) {
            previous = current;
            hasPrevious = true;
            return 0.0;
        }

        uint64_t deltaIdle   = current.idle   - previous.idle;
        uint64_t deltaKernel = current.kernel - previous.kernel;
        uint64_t deltaUser   = current.user   - previous.user;

        uint64_t deltaTotal = deltaKernel + deltaUser;

        previous = current;

        if (deltaTotal == 0) return 0.0;

        double usage = 1.0 - (static_cast<double>(deltaIdle) / deltaTotal);
        return usage * 100.0;
    }
};

int main() {
    std::cout << "System Resource Monitor started." << std::endl;
    
    //RAM calculation and print
    MemoryStatus ram = getRamStatus();
    uint64_t usedBytes = (ram.totalBytes - ram.availableBytes);
    double ramPercent = (static_cast<double>(usedBytes) / ram.totalBytes) * 100;
    std::cout << "Total GB is: " << (ram.totalBytes / 1024 / 1024 / 1024) << std::endl;
    std::cout << "Available GB is: " << (ram.availableBytes / 1024 / 1024 / 1024) << std::endl;
    std::cout << "Percent used: " << ramPercent << std::endl;

    CpuMonitor cpu;
    Sleep(1000); // Wait 1 second for first real check
    double cpuPercent = cpu.getUsage();
    Sleep(1000); // wait 1 second betweeen checks
    cpuPercent = cpu.getUsage();
    std::cout << "CPU Usage: " << cpuPercent << "%" << std::endl;

    return 0;
}