#include <iostream>
#include <windows.h>
#include <iomanip>
#include <csignal>
#include <algorithm>
#include <unordered_set>

#include "SystemMonitor.h"
#include "ConfigManager.h"

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
      processMonitor(
          config.cpuSpikeThreshold(),
          config.ramLeakThreshold(),
          config.historySize()
      ),
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

    while (running)
    {
        constexpr int nameWidth = 30;
        constexpr int pidWidth = 8;
        constexpr int cpuWidth = 10;
        constexpr int ramWidth = 12;

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

        logger.logSystem(cpuPercent, mem.percentUsed, mem.usedBytes);

        auto alerts = detector.check(cpuPercent, mem.percentUsed);

        auto processResult = processMonitor.getProcesses();
        auto processes = processResult.processes;

        alerts.insert(
            alerts.end(),
            processResult.alerts.begin(),
            processResult.alerts.end()
        );

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
        size_t topRamCount = std::min<size_t>(5, processes.size());
        uint64_t totalRamBytes = mem.totalBytes;
        std::unordered_set<DWORD> highlightedPids;
        std::vector<ProcessInfo> topCpuProcesses(
            processesCpu.begin(),
            processesCpu.begin() + topCpuCount
        );
        std::vector<ProcessInfo> topRamProcesses(
            processes.begin(),
            processes.begin() + topRamCount
        );

        logger.logProcesses(topCpuProcesses, topRamProcesses);

        auto printProcessTableHeader = [&]()
        {
            std::cout << std::left
                    << std::setw(nameWidth) << "Name"
                    << std::right
                    << std::setw(pidWidth) << "PID"
                    << std::setw(cpuWidth) << "CPU%"
                    << std::setw(ramWidth) << "RAM MB"
                    << "\n";

            std::cout << std::left
                    << std::setw(nameWidth) << std::string(nameWidth - 1, '-')
                    << std::right
                    << std::setw(pidWidth) << std::string(pidWidth - 1, '-')
                    << std::setw(cpuWidth) << std::string(cpuWidth - 1, '-')
                    << std::setw(ramWidth) << std::string(ramWidth - 1, '-')
                    << "\n";
        };

        auto printProcessRow = [&](const ProcessInfo& p)
        {
            double ramMb =
                static_cast<double>(p.ramBytes) / 1024.0 / 1024.0;

            std::cout << std::left
                    << std::setw(nameWidth) << p.name.substr(0, nameWidth - 1)
                    << std::right
                    << std::setw(pidWidth) << p.pid
                    << std::setw(cpuWidth) << p.cpuPercent
                    << std::setw(ramWidth) << ramMb
                    << "\n";
        };

        printProcessTableHeader();

        for (const auto& p : topCpuProcesses)
        {
            highlightedPids.insert(p.pid);

            printProcessRow(p);
        }

        std::cout << "\n===== TOP 5 RAM CONSUMERS =====\n";

        printProcessTableHeader();

        for (const auto& p : topRamProcesses)
        {
            highlightedPids.insert(p.pid);

            printProcessRow(p);
        }

        std::cout << "\n===== OTHER PROCESSES > 1% CPU OR RAM =====\n";

        for (const auto& p : processes)
        {
            if (highlightedPids.count(p.pid) > 0)
            {
                continue;
            }

            double ramPercent =
                totalRamBytes > 0
                    ? (static_cast<double>(p.ramBytes) /
                       static_cast<double>(totalRamBytes)) * 100.0
                    : 0.0;
            bool ramOverThreshold = ramPercent > 1.0;
            bool cpuOverThreshold = p.cpuPercent > 1.0;

            if (ramOverThreshold || cpuOverThreshold)
            {
                std::string metricsLabel;

                if (cpuOverThreshold && ramOverThreshold)
                {
                    metricsLabel = "CPU & RAM";
                }
                else if (cpuOverThreshold)
                {
                    metricsLabel = "CPU";
                }
                else
                {
                    metricsLabel = "RAM";
                }

                std::cout << p.name
                        << " (PID: " << p.pid << ") "
                        << "[" << metricsLabel << "] "
                        << "CPU: " << p.cpuPercent << " %, "
                        << "RAM: " << ramPercent << " %\n";
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
