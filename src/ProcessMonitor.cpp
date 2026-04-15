#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include "ProcessMonitor.h"

static uint64_t fileTimeToUInt64(const FILETIME& ft)
{
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) |
           ft.dwLowDateTime;
}

std::vector<ProcessInfo> ProcessMonitor::getProcesses()
{
    std::vector<ProcessInfo> processes;

    // Get system CPU time
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    uint64_t systemTime =
        fileTimeToUInt64(kernelTime) +
        fileTimeToUInt64(userTime);

    // Snapshot of processes
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return processes;

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &entry))
    {
        do
        {
            HANDLE processHandle = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE,
                entry.th32ProcessID
            );

            if (processHandle)
            {
                ProcessInfo info;
                info.name = entry.szExeFile;
                info.pid  = entry.th32ProcessID;

                // RAM
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc)))
                {
                    info.ramBytes = pmc.WorkingSetSize;
                }

                // CPU
                FILETIME createTime, exitTime, kernelProcTime, userProcTime;

                if (GetProcessTimes(processHandle,
                                    &createTime,
                                    &exitTime,
                                    &kernelProcTime,
                                    &userProcTime))
                {
                    uint64_t processTime =
                        fileTimeToUInt64(kernelProcTime) +
                        fileTimeToUInt64(userProcTime);

                    uint64_t previousTime =
                        previousProcessTimes[entry.th32ProcessID];

                    double cpuPercent = 0.0;

                    if (previousSystemTime > 0)
                    {
                        uint64_t deltaProcess =
                            processTime - previousTime;

                        uint64_t deltaSystem =
                            systemTime - previousSystemTime;

                        if (deltaSystem > 0)
                        {
                            cpuPercent =
                                (static_cast<double>(deltaProcess) /
                                 static_cast<double>(deltaSystem)) * 100.0;
                        }
                    }

                    info.cpuPercent = cpuPercent;

                    previousProcessTimes[entry.th32ProcessID] = processTime;
                }

                processes.push_back(info);

                CloseHandle(processHandle);
            }

        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);

    // Update systemhistory
    previousSystemTime = systemTime;

    return processes;
}