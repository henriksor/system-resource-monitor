#include <iostream>
#include <windows.h>
#include <cstdint>

struct MemoryMonitor {
public:
    struct MemoryStatus {
        uint64_t totalBytes;
        uint64_t availableBytes;
        uint64_t usedBytes;
        double percentUsed;
    };

    MemoryStatus getStatus() const {
    
    MemoryStatus status;
    
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

    status.totalBytes = statex.ullTotalPhys;
    status.availableBytes = statex.ullAvailPhys;
    status.usedBytes = (status.totalBytes - status.availableBytes);
    status.percentUsed = (static_cast<double>(status.usedBytes) / status.totalBytes) * 100;

    return status;
    };
};
