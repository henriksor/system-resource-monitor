#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>

#include "Alert.h"
#include "ProcessInfo.h"

class Logger {
public:
    explicit Logger(const std::string& filename);

    void logSystem(double cpuPercent, double ramPercent, uint64_t ramUsedBytes);
    void logProcesses(
        const std::vector<ProcessInfo>& topCpuProcesses,
        const std::vector<ProcessInfo>& topRamProcesses
    );
    void logAlert(const Alert& alert);

private:
    std::ofstream openCsvFile(
        const std::filesystem::path& path,
        const std::string& header
    ) const;
    std::filesystem::path deriveSiblingPath(
        const std::string& filename
    ) const;
    void logProcessGroup(
        const std::string& category,
        const std::vector<ProcessInfo>& processes
    );

    std::filesystem::path systemLogPath;
    std::ofstream systemFile;
    std::ofstream processFile;
    std::ofstream alertFile;
};
