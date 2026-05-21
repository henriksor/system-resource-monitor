#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <cstdint>
#include <vector>

#include "Alert.h"
#include "ProcessInfo.h"

class Logger {
public:
    explicit Logger(const std::string& filename);

    void logSystem(
        const std::optional<double>& cpuPercent,
        const std::optional<double>& ramPercent,
        const std::optional<uint64_t>& ramUsedBytes
    );
    void logProcesses(
        const std::vector<ProcessInfo>& topCpuProcesses,
        const std::vector<ProcessInfo>& topRamProcesses
    );
    void logAlert(const Alert& alert);
    void flush();

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
    void ensureWritable(
        const std::ofstream& file,
        const std::filesystem::path& path
    ) const;

    std::filesystem::path systemLogPath;
    std::filesystem::path processLogPath;
    std::filesystem::path alertLogPath;
    std::ofstream systemFile;
    std::ofstream processFile;
    std::ofstream alertFile;
};
