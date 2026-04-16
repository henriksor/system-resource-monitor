#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

#include "ProcessMonitor.h"

static uint64_t fileTimeToUInt64(const FILETIME& ft)
{
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) |
           ft.dwLowDateTime;
}

static std::string buildProcessAlertMessage(
    const ProcessInfo& process,
    const std::string& metric,
    double value,
    const std::string& description
)
{
    std::ostringstream stream;
    stream << description
           << " [PID=" << process.pid
           << ", Name=" << process.name
           << ", Metric=" << metric
           << ", Value=" << std::fixed << std::setprecision(2) << value
           << "]";
    return stream.str();
}

ProcessMonitor::ProcessMonitor(
    double cpuSpikeThreshold,
    double ramLeakThresholdMb,
    size_t historySize
)
    : cpuSpikeThreshold(cpuSpikeThreshold),
      ramLeakThresholdBytes(ramLeakThresholdMb * 1024.0 * 1024.0),
      maxHistory(std::max<size_t>(historySize, 2))
{
}

ProcessMonitorResult ProcessMonitor::getProcesses()
{
    ProcessMonitorResult result;
    std::unordered_set<DWORD> activePids;

    // Get system CPU time
    FILETIME idleTime, kernelTime, userTime;
    bool hasSystemTimes =
        GetSystemTimes(&idleTime, &kernelTime, &userTime) != FALSE;

    uint64_t systemTime =
        hasSystemTimes
            ? fileTimeToUInt64(kernelTime) + fileTimeToUInt64(userTime)
            : 0;

    MEMORYSTATUSEX memoryStatus = {};
    memoryStatus.dwLength = sizeof(memoryStatus);
    uint64_t totalPhysicalMemory = 0;
    if (GlobalMemoryStatusEx(&memoryStatus))
    {
        totalPhysicalMemory = memoryStatus.ullTotalPhys;
    }

    // Snapshot of processes
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return result;

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &entry))
    {
        do
        {
            activePids.insert(entry.th32ProcessID);

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
                uint64_t currentRamBytes = 0;
                bool hasMemoryInfo = false;

                FILETIME createTime, exitTime, kernelProcTime, userProcTime;
                bool hasProcessTimes =
                    GetProcessTimes(processHandle,
                                    &createTime,
                                    &exitTime,
                                    &kernelProcTime,
                                    &userProcTime) != FALSE;
                bool sameProcessInstance = false;

                if (hasProcessTimes)
                {
                    uint64_t currentCreateTime = fileTimeToUInt64(createTime);
                    auto previousCreateTimeIt =
                        previousProcessCreateTimes.find(entry.th32ProcessID);
                    sameProcessInstance =
                        previousCreateTimeIt !=
                            previousProcessCreateTimes.end() &&
                        previousCreateTimeIt->second == currentCreateTime;

                    if (!sameProcessInstance)
                    {
                        previousProcessTimes.erase(entry.th32ProcessID);
                        previousProcessCreateTimes.erase(entry.th32ProcessID);
                        cpuHistoryByPid.erase(entry.th32ProcessID);
                        ramHistoryByPid.erase(entry.th32ProcessID);
                    }

                    previousProcessCreateTimes[entry.th32ProcessID] =
                        currentCreateTime;
                }
                else
                {
                    previousProcessTimes.erase(entry.th32ProcessID);
                    previousProcessCreateTimes.erase(entry.th32ProcessID);
                    cpuHistoryByPid.erase(entry.th32ProcessID);
                    ramHistoryByPid.erase(entry.th32ProcessID);
                }

                // RAM
                PROCESS_MEMORY_COUNTERS pmc{};
                if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc)))
                {
                    hasMemoryInfo = true;
                    currentRamBytes = pmc.WorkingSetSize;
                    info.ramBytes = currentRamBytes;
                }

                double ramPercent = 0.0;
                if (totalPhysicalMemory > 0)
                {
                    ramPercent =
                        (static_cast<double>(info.ramBytes) /
                         static_cast<double>(totalPhysicalMemory)) * 100.0;
                }

                if (hasProcessTimes)
                {
                    uint64_t processTime =
                        fileTimeToUInt64(kernelProcTime) +
                        fileTimeToUInt64(userProcTime);

                    auto previousTimeIt =
                        previousProcessTimes.find(entry.th32ProcessID);
                    uint64_t previousTime =
                        previousTimeIt != previousProcessTimes.end()
                            ? previousTimeIt->second
                            : 0;

                    double cpuPercent = 0.0;

                    bool hasValidCpuBaseline =
                        sameProcessInstance &&
                        hasSystemTimes &&
                        previousSystemTime > 0 &&
                        previousTimeIt != previousProcessTimes.end();

                    if (hasValidCpuBaseline)
                    {
                        if (processTime < previousTime)
                        {
                            previousProcessTimes[entry.th32ProcessID] =
                                processTime;
                            cpuHistoryByPid.erase(entry.th32ProcessID);
                        }
                        else
                        {
                            uint64_t deltaProcess =
                                processTime - previousTime;

                            uint64_t deltaSystem =
                                systemTime - previousSystemTime;

                            if (deltaSystem > 0)
                            {
                                cpuPercent =
                                    (static_cast<double>(deltaProcess) /
                                     static_cast<double>(deltaSystem)) *
                                    100.0;
                            }
                        }
                    }

                    info.cpuPercent = cpuPercent;

                    auto& cpuHistory = cpuHistoryByPid[entry.th32ProcessID];
                    if (!hasValidCpuBaseline)
                    {
                        // A missing system-time baseline means cpuPercent is
                        // only a safe fallback value, not a real measurement.
                        // Clear stale history so the next valid sample does not
                        // compare against an artificial zero and trigger a
                        // false spike alert.
                        cpuHistory.clear();
                    }
                    else
                    {
                        if (!cpuHistory.empty())
                        {
                            double cpuDiff = cpuPercent - cpuHistory.back();

                            if (cpuDiff > cpuSpikeThreshold)
                            {
                                result.alerts.push_back({
                                    "Process CPU",
                                    AlertType::Spike,
                                    Severity::Warning,
                                    cpuPercent,
                                    cpuDiff,
                                    buildProcessAlertMessage(
                                        info,
                                        "Process CPU",
                                        cpuPercent,
                                        "CPU spike detected"
                                    )
                                });
                            }
                        }

                        // CPU visibility should not depend on memory
                        // footprint, but spike history must only use validated
                        // measurements with a real system-time baseline.
                        cpuHistory.push_back(cpuPercent);
                        if (cpuHistory.size() > maxHistory)
                        {
                            cpuHistory.pop_front();
                        }
                    }

                    previousProcessTimes[entry.th32ProcessID] = processTime;
                }

                if (hasMemoryInfo && hasProcessTimes)
                {
                    auto& ramHistory = ramHistoryByPid[entry.th32ProcessID];
                    ramHistory.push_back(currentRamBytes);
                    if (ramHistory.size() > maxHistory)
                    {
                        ramHistory.pop_front();
                    }

                    if (ramHistory.size() >= maxHistory)
                    {
                        bool consistentlyIncreasing = true;
                        uint64_t totalGrowthBytes = 0;

                        for (size_t i = 1; i < ramHistory.size(); ++i)
                        {
                            if (ramHistory[i] <= ramHistory[i - 1])
                            {
                                consistentlyIncreasing = false;
                                break;
                            }

                            totalGrowthBytes +=
                                ramHistory[i] - ramHistory[i - 1];
                        }

                        if (consistentlyIncreasing)
                        {
                            double averageGrowthBytes =
                                static_cast<double>(totalGrowthBytes) /
                                static_cast<double>(ramHistory.size() - 1);

                            if (averageGrowthBytes > ramLeakThresholdBytes)
                            {
                                result.alerts.push_back({
                                    "Process RAM",
                                    AlertType::Leak,
                                    Severity::Critical,
                                    static_cast<double>(info.ramBytes) /
                                        1024.0 / 1024.0,
                                    averageGrowthBytes / 1024.0 / 1024.0,
                                    buildProcessAlertMessage(
                                        info,
                                        "Process RAM",
                                        static_cast<double>(info.ramBytes) /
                                            1024.0 / 1024.0,
                                        "RAM leak detected"
                                    )
                                });
                            }
                        }
                    }
                }

                result.processes.push_back(info);

                CloseHandle(processHandle);
            }

        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);

    for (auto it = previousProcessTimes.begin();
         it != previousProcessTimes.end();)
    {
        if (activePids.find(it->first) == activePids.end())
        {
            it = previousProcessTimes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = previousProcessCreateTimes.begin();
         it != previousProcessCreateTimes.end();)
    {
        if (activePids.find(it->first) == activePids.end())
        {
            it = previousProcessCreateTimes.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = cpuHistoryByPid.begin();
         it != cpuHistoryByPid.end();)
    {
        if (activePids.find(it->first) == activePids.end())
        {
            it = cpuHistoryByPid.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = ramHistoryByPid.begin();
         it != ramHistoryByPid.end();)
    {
        if (activePids.find(it->first) == activePids.end())
        {
            it = ramHistoryByPid.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Update systemhistory
    previousSystemTime = systemTime;

    return result;
}
