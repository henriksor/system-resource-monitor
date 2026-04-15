#include "ProcessMonitor.h"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

std::vector<ProcessInfo> ProcessMonitor::getProcesses()
{
    std::vector<ProcessInfo> processes;

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
                PROCESS_MEMORY_COUNTERS pmc;

                if (GetProcessMemoryInfo(processHandle, &pmc, sizeof(pmc)))
                {
                    ProcessInfo info;
                    info.name = entry.szExeFile;
                    info.pid = entry.th32ProcessID;
                    info.ramBytes = pmc.WorkingSetSize;

                    processes.push_back(info);
                }

                CloseHandle(processHandle);
            }

        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);

    return processes;
}