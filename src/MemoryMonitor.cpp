#include <windows.h>
#include "MemoryMonitor.h"

MemoryMonitor::MemoryStatus MemoryMonitor::getStatus() const
{
    MemoryStatus status{};

    MEMORYSTATUSEX statex{};
    statex.dwLength = sizeof(statex);

    if (GlobalMemoryStatusEx(&statex) == FALSE)
    {
        // Return a zeroed status when the OS query fails so callers do not
        // accidentally use uninitialized memory as if it were real telemetry.
        return status;
    }

    status.totalBytes = statex.ullTotalPhys;
    status.availableBytes = statex.ullAvailPhys;
    status.usedBytes = status.totalBytes - status.availableBytes;
    status.percentUsed =
        status.totalBytes > 0
            ? (static_cast<double>(status.usedBytes) / status.totalBytes) *
                  100.0
            : 0.0;

    return status;
}
