#include <iostream>
#include <windows.h>
#include <cstdint>

struct  MemoryStatus {
        uint64_t totalBytes;
        uint64_t availableBytes;
    };

//RAM check
MemoryStatus getRamStatus() {
    
    MemoryStatus status;
    
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

    status.totalBytes = statex.ullTotalPhys;
    status.availableBytes = statex.ullAvailPhys;

    return status;
}

int main() {
    std::cout << "System Resource Monitor started." << std::endl;
    
    //RAM check and print
    MemoryStatus ram = getRamStatus();
    uint64_t usedBytes = (ram.totalBytes - ram.availableBytes);
    double ramPercent = (static_cast<double>(usedBytes) / ram.totalBytes) * 100;
    std::cout << "Total GB is: " << (ram.totalBytes / 1024 / 1024 / 1024) << std::endl;
    std::cout << "Available GB is: " << (ram.availableBytes / 1024 / 1024 / 1024) << std::endl;
    std::cout << "Percent used: " << ramPercent << std::endl;

    return 0;
}