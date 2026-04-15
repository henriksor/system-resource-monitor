#include <iostream>
#include <windows.h>
#include <iomanip>
#include <csignal>
#include <algorithm>

#include "SystemMonitor.h"
#include "ConfigManager.h"
#include "ProcessMonitor.h"

// pointer used for Ctrl+C handling
static SystemMonitor* instance = nullptr;

// Windows console signal handler
BOOL WINAPI consoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT && instance)
    {
        std::cout << "\nCtrl+C detected. Shutting down...\n";
        instance->stop();
        return TRUE;
    }
    return FALSE;
}

void SystemMonitor::warmUp()
{
    cpu.getUsage();
    Sleep(1000);
}

void SystemMonitor::stop()
{
    running = false;
}

SystemMonitor::SystemMonitor()
    : config("config.json"),
      logger(config.logFile()),
      detector(
          config.cpuThreshold(),
          config.ramThreshold(),
          config.cpuSpikeThreshold(),
          config.ramSpikeThreshold(),
          config.cpuLeakThreshold(),
          config.ramLeakThreshold(),
          config.historySize()
      )
{
}

void SystemMonitor::run()
{
    // Register Ctrl+C handler
    instance = this;
    SetConsoleCtrlHandler(consoleHandler, TRUE);
    
    warmUp();

    ProcessMonitor processMonitor;

    while (running)
    {
        // Clear screen using ANSI escape codes
        std::cout << "\x1B[2J\x1B[H";

        auto mem = memory.getStatus();
        double cpuPercent = cpu.getUsage();

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "CPU Usage: " << cpuPercent << " %\n\n";

        std::cout << "Memory Usage: " << mem.percentUsed << " %\n";
        std::cout << "Total: " << mem.totalBytes / 1024.0 / 1024.0 / 1024.0 << " GB\n";

        std::cout << "Available: " << mem.availableBytes / 1024.0 / 1024.0 / 1024.0 << " GB\n";

        std::cout << "Used: " << mem.usedBytes / 1024.0 / 1024.0 / 1024.0 << " GB\n";

        logger.log(cpuPercent, mem.percentUsed, mem.usedBytes);

        auto alerts = detector.check(cpuPercent, mem.percentUsed);

        auto processes = processMonitor.getProcesses();

        auto processesCpu = processes;

        std::sort(processesCpu.begin(), processesCpu.end(),
            [](const ProcessInfo& a, const ProcessInfo& b)
            {
                return a.cpuPercent > b.cpuPercent;
            });

        std::sort(processes.begin(), processes.end(),
            [](const ProcessInfo& a, const ProcessInfo& b)
            {
                return a.ramBytes > b.ramBytes;
            });


        std::cout << "\n===== TOP 5 CPU =====\n";

        size_t topCpuCount = std::min<size_t>(5, processesCpu.size());

        for (size_t i = 0; i < topCpuCount; ++i)
        {
            const auto& p = processesCpu[i];

            std::cout << p.name
                    << " (PID: " << p.pid << ") "
                    << p.cpuPercent << " %\n";
        }

        std::cout << "\n===== TOP 5 RAM CONSUMERS =====\n";

        size_t topCount = std::min<size_t>(5, processes.size());

        for (size_t i = 0; i < topCount; ++i)
        {
            const auto& p = processes[i];

            std::cout << p.name
                    << " (PID: " << p.pid << ") "
                    << p.ramBytes / 1024.0 / 1024.0
                    << " MB\n";
        }

        std::cout << "\n===== OTHER PROCESSES > 1% RAM =====\n";

        uint64_t totalRamBytes = mem.totalBytes;

        for (size_t i = topCount; i < processes.size(); ++i)
        {
            const auto& p = processes[i];

            double percent =
                (static_cast<double>(p.ramBytes) /
                static_cast<double>(totalRamBytes)) * 100.0;

            if (percent > 1.0)
            {
                std::cout << p.name
                        << " (PID: " << p.pid << ") "
                        << percent << " %\n";
            }
        }

        for (const auto& alert : alerts)
        {
            switch (alert.severity)
            {
                case Severity::Info:
                    std::cout << "\x1B[34m";
                    break;
                case Severity::Warning:
                    std::cout << "\x1B[33m";
                    break;
                case Severity::Critical:
                    std::cout << "\x1B[31m";
                    break;
            }

            std::cout << alert.message
                    << " (" << alert.metric << ": "
                    << alert.value << ")\n";

            std::cout << "\x1B[0m";

            logger.logAlert(alert);
        }

        Sleep(1000);
    }
    std::cout << "System monitor stopped.\n";
}