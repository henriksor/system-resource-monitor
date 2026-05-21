#include "OutputHandler.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <windows.h>

#include "SnapshotJson.h"

namespace {
struct RankedProcesses {
    std::vector<ProcessInfo> topCpuProcesses;
    std::vector<ProcessInfo> topRamProcesses;
};

RankedProcesses rankProcesses(
    const std::vector<ProcessInfo>& processes,
    size_t limit = 5
)
{
    RankedProcesses ranked;

    auto processesByCpu = processes;
    auto processesByRam = processes;

    std::sort(processesByCpu.begin(), processesByCpu.end(),
        [](const ProcessInfo& a, const ProcessInfo& b)
        {
            return a.cpuPercent > b.cpuPercent;
        });

    std::sort(processesByRam.begin(), processesByRam.end(),
        [](const ProcessInfo& a, const ProcessInfo& b)
        {
            return a.ramBytes > b.ramBytes;
        });

    size_t topCpuCount = std::min(limit, processesByCpu.size());
    size_t topRamCount = std::min(limit, processesByRam.size());

    ranked.topCpuProcesses.assign(
        processesByCpu.begin(),
        processesByCpu.begin() + topCpuCount
    );
    ranked.topRamProcesses.assign(
        processesByRam.begin(),
        processesByRam.begin() + topRamCount
    );

    return ranked;
}
} // namespace

void ConsoleOutputHandler::handle(const SystemSnapshot& snapshot)
{
    constexpr int nameWidth = 30;
    constexpr int pidWidth = 8;
    constexpr int cpuWidth = 10;
    constexpr int ramWidth = 12;

    const auto rankedProcesses = rankProcesses(snapshot.processes);
    std::unordered_set<DWORD> highlightedPids;

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

    auto printProcessRow = [&](const ProcessInfo& process)
    {
        double ramMb =
            static_cast<double>(process.ramBytes) / 1024.0 / 1024.0;

        std::cout << std::left
                << std::setw(nameWidth)
                << process.name.substr(0, nameWidth - 1)
                << std::right
                << std::setw(pidWidth) << process.pid
                << std::setw(cpuWidth) << process.cpuPercent
                << std::setw(ramWidth) << ramMb
                << "\n";
    };

    std::cout << "\x1B[2J\x1B[H";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "CPU Usage: ";
    if (snapshot.cpuPercent.has_value())
    {
        std::cout << *snapshot.cpuPercent << " %\n\n";
    }
    else
    {
        std::cout << "N/A\n\n";
    }

    std::cout << "Memory Usage: ";
    if (snapshot.ramPercent.has_value())
    {
        std::cout << *snapshot.ramPercent << " %\n";
    }
    else
    {
        std::cout << "N/A\n";
    }

    if (snapshot.memory.has_value())
    {
        std::cout << "Total: "
                  << snapshot.memory->totalBytes / 1024.0 / 1024.0 / 1024.0
                  << " GB\n";
        std::cout << "Available: "
                  << snapshot.memory->availableBytes / 1024.0 / 1024.0 / 1024.0
                  << " GB\n";
        std::cout << "Used: "
                  << snapshot.memory->usedBytes / 1024.0 / 1024.0 / 1024.0
                  << " GB\n";
    }
    else
    {
        std::cout << "Total: N/A\n";
        std::cout << "Available: N/A\n";
        std::cout << "Used: N/A\n";
    }

    std::cout << "\n===== TOP 5 CPU =====\n";

    printProcessTableHeader();
    for (const auto& process : rankedProcesses.topCpuProcesses)
    {
        highlightedPids.insert(process.pid);
        printProcessRow(process);
    }

    std::cout << "\n===== TOP 5 RAM CONSUMERS =====\n";

    printProcessTableHeader();
    for (const auto& process : rankedProcesses.topRamProcesses)
    {
        highlightedPids.insert(process.pid);
        printProcessRow(process);
    }

    std::cout << "\n===== OTHER PROCESSES > 1% CPU OR RAM =====\n";

    for (const auto& process : snapshot.processes)
    {
        if (highlightedPids.count(process.pid) > 0)
        {
            continue;
        }

        double ramPercent =
            snapshot.memory.has_value() && snapshot.memory->totalBytes > 0
                ? (static_cast<double>(process.ramBytes) /
                   static_cast<double>(snapshot.memory->totalBytes)) * 100.0
                : 0.0;
        bool ramOverThreshold = ramPercent > 1.0;
        bool cpuOverThreshold = process.cpuPercent > 1.0;

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

            std::cout << process.name
                    << " (PID: " << process.pid << ") "
                    << "[" << metricsLabel << "] "
                    << "CPU: " << process.cpuPercent << " %, "
                    << "RAM: " << ramPercent << " %\n";
        }
    }

    for (const auto& alert : snapshot.alerts)
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
    }
}

CsvLoggerOutputHandler::CsvLoggerOutputHandler(const std::string& filename)
    : logger(filename)
{
}

void CsvLoggerOutputHandler::handle(const SystemSnapshot& snapshot)
{
    logger.logSystem(
        snapshot.cpuPercent,
        snapshot.ramPercent,
        snapshot.memory.has_value()
            ? std::optional<uint64_t>(snapshot.memory->usedBytes)
            : std::nullopt
    );

    const auto rankedProcesses = rankProcesses(snapshot.processes);
    logger.logProcesses(
        rankedProcesses.topCpuProcesses,
        rankedProcesses.topRamProcesses
    );

    for (const auto& alert : snapshot.alerts)
    {
        logger.logAlert(alert);
    }

    logger.flush();
}

JsonSnapshotOutputHandler::JsonSnapshotOutputHandler(
    std::filesystem::path outputPath
)
    : outputPath(std::move(outputPath))
{
}

void JsonSnapshotOutputHandler::handle(const SystemSnapshot& snapshot)
{
    const auto parent = outputPath.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }

    auto tempPath = outputPath;
    tempPath.replace_extension(".tmp");

    std::ofstream file(tempPath, std::ios::trunc);
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Could not open JSON snapshot file: " + tempPath.string()
        );
    }

    nlohmann::json jsonSnapshot = snapshot;
    file << std::setw(2) << jsonSnapshot << '\n';

    if (!file.good())
    {
        file.close();
        std::filesystem::remove(tempPath);
        throw std::runtime_error(
            "Failed to write JSON snapshot file: " + tempPath.string()
        );
    }

    file.close();
    if (file.fail())
    {
        std::filesystem::remove(tempPath);
        throw std::runtime_error(
            "Failed to finalize JSON snapshot file: " + tempPath.string()
        );
    }

    if (!MoveFileExW(
            tempPath.wstring().c_str(),
            outputPath.wstring().c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        ))
    {
        std::filesystem::remove(tempPath);
        throw std::runtime_error(
            "Could not atomically replace JSON snapshot file: " +
            outputPath.string()
        );
    }
}
