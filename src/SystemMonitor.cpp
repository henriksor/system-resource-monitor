#include <iostream>
#include <windows.h>
#include <iomanip>
#include <csignal>

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