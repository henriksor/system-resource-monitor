#include <windows.h>
#include "MemoryMonitor.h"

MemoryMonitor::MemoryStatus MemoryMonitor::getStatus() const
{
    MemoryStatus status;

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);

    GlobalMemoryStatusEx(&statex);

    status.totalBytes = statex.ullTotalPhys;
    status.availableBytes = statex.ullAvailPhys;
    status.usedBytes = status.totalBytes - status.availableBytes;
    status.percentUsed =
        (static_cast<double>(status.usedBytes) / status.totalBytes) * 100.0;

    return status;
}