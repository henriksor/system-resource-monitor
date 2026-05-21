#pragma once

#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "AlertHandler.h"
#include "AnomalyDetector.h"
#include "ConfigManager.h"
#include "OutputHandler.h"
#include "ProcessMonitor.h"
#include "SystemSnapshot.h"

#include <atomic>
#include <deque>
#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

class SystemMonitor {
public:
    SystemMonitor();
    ~SystemMonitor();

    void run();
    void stop();

private:
    static HANDLE acquireSingleInstanceLock();

    void warmUp();
    SystemSnapshot collectSnapshot();
    void dispatchAlerts(const SystemSnapshot& snapshot);
    void pruneCompletedAlertTasks();
    void waitForAlertTasks();
    void launchAlertTask(const Alert& alert, AlertHandler* handler);
    void drainPendingAlerts();
    void enqueuePendingAlert(const Alert& alert, AlertHandler* handler);
    void logDroppedAlert(const Alert& alert, const std::string& reason) const;

    struct PendingAlert {
        Alert alert;
        AlertHandler* handler;
    };

    std::atomic<bool> running{true};

    HANDLE singleInstanceMutex = nullptr;
    std::filesystem::path runtimeRoot;
    ConfigManager config;
    CpuMonitor cpu;
    MemoryMonitor memory;
    ProcessMonitor processMonitor;
    AnomalyDetector detector;
    std::vector<std::unique_ptr<OutputHandler>> outputHandlers;
    std::vector<std::unique_ptr<AlertHandler>> alertHandlers;
    std::vector<std::future<void>> alertTasks;
    std::deque<PendingAlert> pendingAlerts;
};
