#include <iostream>
#include <windows.h>
#include <algorithm>
#include <csignal>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <memory>

#include "SystemMonitor.h"
#include "ConfigManager.h"

namespace {
constexpr size_t MAX_ALERT_TASKS = 8;
constexpr size_t MAX_PENDING_ALERTS = 64;

std::filesystem::path getExecutablePath()
{
    std::wstring buffer(MAX_PATH, L'\0');

    while (true)
    {
        DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );

        if (length == 0)
        {
            throw std::runtime_error("Could not resolve executable path");
        }

        if (length < buffer.size() - 1)
        {
            buffer.resize(length);
            return std::filesystem::path(buffer);
        }

        buffer.resize(buffer.size() * 2);
    }
}

std::filesystem::path findFileInParents(
    const std::filesystem::path& start,
    const std::filesystem::path& relativePath
)
{
    auto current = std::filesystem::absolute(start);

    while (!current.empty())
    {
        const auto candidate = current / relativePath;
        if (std::filesystem::exists(candidate))
        {
            return candidate;
        }

        if (current == current.root_path())
        {
            break;
        }

        current = current.parent_path();
    }

    return {};
}

std::filesystem::path resolveRuntimeRoot()
{
    const auto executableDirectory = getExecutablePath().parent_path();

    if (const auto configPath =
            findFileInParents(executableDirectory, "config.json");
        !configPath.empty())
    {
        return configPath.parent_path();
    }

    if (const auto configPath =
            findFileInParents(std::filesystem::current_path(), "config.json");
        !configPath.empty())
    {
        return configPath.parent_path();
    }

    throw std::runtime_error("Could not locate config.json");
}

std::string getEnvironmentVariable(const char* name)
{
    const char* value = std::getenv(name);
    return value != nullptr ? value : "";
}

const char* severityName(Severity severity)
{
    switch (severity)
    {
        case Severity::Info:
            return "Info";
        case Severity::Warning:
            return "Warning";
        case Severity::Critical:
            return "Critical";
    }

    return "Unknown";
}
} // namespace

static SystemMonitor* instance = nullptr;

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

HANDLE SystemMonitor::acquireSingleInstanceLock()
{
    HANDLE mutex = CreateMutexW(
        nullptr,
        TRUE,
        L"Local\\SystemResourceMonitor.SingleInstance"
    );

    if (mutex == nullptr)
    {
        throw std::runtime_error(
            "Could not create single-instance mutex"
        );
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(mutex);
        throw std::runtime_error(
            "Another SystemResourceMonitor instance is already running"
        );
    }

    return mutex;
}

void SystemMonitor::launchAlertTask(const Alert& alert, AlertHandler* handler)
{
    alertTasks.push_back(std::async(
        std::launch::async,
        [handler, alert]()
        {
            handler->handle(alert);
        }
    ));
}

void SystemMonitor::dispatchAlerts(const SystemSnapshot& snapshot)
{
    pruneCompletedAlertTasks();
    drainPendingAlerts();

    for (const auto& alert : snapshot.alerts)
    {
        if (alert.severity == Severity::Info)
        {
            continue;
        }

        for (const auto& alertHandler : alertHandlers)
        {
            pruneCompletedAlertTasks();
            drainPendingAlerts();

            if (alertTasks.size() >= MAX_ALERT_TASKS)
            {
                enqueuePendingAlert(alert, alertHandler.get());
                continue;
            }

            launchAlertTask(alert, alertHandler.get());
        }
    }
}

void SystemMonitor::enqueuePendingAlert(
    const Alert& alert,
    AlertHandler* handler
)
{
    if (pendingAlerts.size() < MAX_PENDING_ALERTS)
    {
        pendingAlerts.push_back(PendingAlert{alert, handler});
        return;
    }

    if (alert.severity == Severity::Critical)
    {
        // Critical alerts get the remaining queue capacity first, but the
        // sampling loop must never block on external delivery.
        const auto warningIt = std::find_if(
            pendingAlerts.begin(),
            pendingAlerts.end(),
            [](const PendingAlert& pendingAlert)
            {
                return pendingAlert.alert.severity == Severity::Warning;
            }
        );

        if (warningIt != pendingAlerts.end())
        {
            logDroppedAlert(
                warningIt->alert,
                "evicted by critical alert priority"
            );
            pendingAlerts.erase(warningIt);
        }
        else
        {
            logDroppedAlert(
                pendingAlerts.front().alert,
                "evicted oldest critical alert due to queue limit"
            );
            pendingAlerts.pop_front();
        }

        pendingAlerts.push_back(PendingAlert{alert, handler});
        return;
    }

    logDroppedAlert(alert, "queue limit reached");
}

void SystemMonitor::logDroppedAlert(
    const Alert& alert,
    const std::string& reason
) const
{
    std::cerr << "[ALERT] Dropped " << severityName(alert.severity)
              << " alert for " << alert.metric
              << ": " << reason << "\n";
}

void SystemMonitor::drainPendingAlerts()
{
    while (alertTasks.size() < MAX_ALERT_TASKS && !pendingAlerts.empty())
    {
        PendingAlert pendingAlert = pendingAlerts.front();
        pendingAlerts.pop_front();
        launchAlertTask(pendingAlert.alert, pendingAlert.handler);
    }
}

void SystemMonitor::pruneCompletedAlertTasks()
{
    alertTasks.erase(
        std::remove_if(
            alertTasks.begin(),
            alertTasks.end(),
            [](std::future<void>& task)
            {
                if (task.wait_for(std::chrono::seconds(0)) !=
                    std::future_status::ready)
                {
                    return false;
                }

                try
                {
                    task.get();
                }
                catch (const std::exception& ex)
                {
                    std::cerr << "Alert handler failed: "
                              << ex.what() << "\n";
                }
                catch (...)
                {
                    std::cerr << "Alert handler failed with an unknown error.\n";
                }

                return true;
            }
        ),
        alertTasks.end()
    );
}

void SystemMonitor::waitForAlertTasks()
{
    for (auto& task : alertTasks)
    {
        try
        {
            task.get();
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Alert handler failed: "
                      << ex.what() << "\n";
        }
        catch (...)
        {
            std::cerr << "Alert handler failed with an unknown error.\n";
        }
    }

    alertTasks.clear();
}

SystemSnapshot SystemMonitor::collectSnapshot()
{
    SystemSnapshot snapshot;

    snapshot.cpuPercent = cpu.getUsage();

    if (const auto memoryStatus = memory.getStatus(); memoryStatus.has_value())
    {
        snapshot.ramPercent = memoryStatus->percentUsed;
        snapshot.memory = SystemSnapshot::MemoryDetails{
            memoryStatus->usedBytes,
            memoryStatus->totalBytes,
            memoryStatus->availableBytes
        };
    }

    snapshot.alerts = detector.check(
        snapshot.cpuPercent,
        snapshot.ramPercent
    );

    auto processResult = processMonitor.getProcesses();
    snapshot.processes = std::move(processResult.processes);
    snapshot.alerts.insert(
        snapshot.alerts.end(),
        processResult.alerts.begin(),
        processResult.alerts.end()
    );

    return snapshot;
}

SystemMonitor::SystemMonitor()
    : singleInstanceMutex(acquireSingleInstanceLock()),
      runtimeRoot(resolveRuntimeRoot()),
      config((runtimeRoot / "config.json").string(), runtimeRoot),
      processMonitor(
          config.cpuSpikeThreshold(),
          config.ramLeakThresholdMb(),
          config.historySize()
      ),
      detector(
          config.cpuThreshold(),
          config.ramThreshold(),
          config.cpuSpikeThreshold(),
          config.ramSpikeThreshold(),
          config.cpuLeakThreshold(),
          config.ramLeakThresholdPercent(),
          config.historySize()
      )
{
    const auto resolvedLogPath = std::filesystem::path(config.logFile());
    const auto snapshotPath = runtimeRoot / "dashboard" / "latest_snapshot.json";

    // Register outputs centrally so collection stays independent of any
    // specific presentation or integration target.
    outputHandlers.push_back(std::make_unique<ConsoleOutputHandler>());
    outputHandlers.push_back(
        std::make_unique<CsvLoggerOutputHandler>(resolvedLogPath.string())
    );
    outputHandlers.push_back(
        std::make_unique<JsonSnapshotOutputHandler>(snapshotPath)
    );

    const std::string webhookUrl =
        getEnvironmentVariable("SYSTEM_MONITOR_WEBHOOK_URL");
    if (!webhookUrl.empty())
    {
        alertHandlers.push_back(
            std::make_unique<WebhookAlertHandler>(webhookUrl)
        );
    }

    const std::string emailRecipient =
        getEnvironmentVariable("SYSTEM_MONITOR_EMAIL_TO");
    if (!emailRecipient.empty())
    {
        alertHandlers.push_back(
            std::make_unique<EmailAlertHandler>(emailRecipient)
        );
    }
}

SystemMonitor::~SystemMonitor()
{
    if (singleInstanceMutex != nullptr)
    {
        CloseHandle(singleInstanceMutex);
        singleInstanceMutex = nullptr;
    }
}

void SystemMonitor::run()
{
    instance = this;
    SetConsoleCtrlHandler(consoleHandler, TRUE);

    warmUp();

    while (running)
    {
        const auto snapshot = collectSnapshot();

        for (const auto& outputHandler : outputHandlers)
        {
            try
            {
                outputHandler->handle(snapshot);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "Output handler failed: " << ex.what() << "\n";
            }
            catch (...)
            {
                std::cerr << "Output handler failed with an unknown error.\n";
            }
        }

        dispatchAlerts(snapshot);
        pruneCompletedAlertTasks();
        drainPendingAlerts();

        Sleep(1000);
    }

    waitForAlertTasks();
    std::cout << "System monitor stopped.\n";
}
