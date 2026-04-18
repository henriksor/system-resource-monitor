#include <windows.h>
#include "MemoryMonitor.h"

std::optional<MemoryMonitor::MemoryStatus> MemoryMonitor::getStatus() const
{
    MemoryStatus status{};

    MEMORYSTATUSEX statex{};
    statex.dwLength = sizeof(statex);

    if (GlobalMemoryStatusEx(&statex) == FALSE)
    {
        return std::nullopt;
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
