#include <windows.h>
#include "CpuMonitor.h"

CpuMonitor::CpuTimes CpuMonitor::getCurrentTimes()
{
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

double CpuMonitor::getUsage()
{
    CpuTimes current = getCurrentTimes();

    if (!hasPrevious)
    {
        previous = current;
        hasPrevious = true;
        return 0.0;
    }

    uint64_t deltaIdle   = current.idle   - previous.idle;
    uint64_t deltaKernel = current.kernel - previous.kernel;
    uint64_t deltaUser   = current.user   - previous.user;

    uint64_t deltaTotal = deltaKernel + deltaUser;

    previous = current;

    if (deltaTotal == 0)
        return 0.0;

    double usage =
        1.0 - (static_cast<double>(deltaIdle) / deltaTotal);

    return usage * 100.0;
}