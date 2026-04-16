#include "Logger.h"

#include <stdexcept>

namespace {
std::string alertTypeToString(AlertType type)
{
    switch (type)
    {
        case AlertType::Threshold:
            return "Threshold";
        case AlertType::Spike:
            return "Spike";
        case AlertType::Leak:
            return "Leak";
    }

    return "Unknown";
}

std::string severityToString(Severity severity)
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

Logger::Logger(const std::string& filename)
    : systemLogPath(filename),
      systemFile(
          openCsvFile(
              systemLogPath,
              "CPU%,RAM%,RAM_Used_Bytes\n"
          )
      ),
      processFile(
          openCsvFile(
              deriveSiblingPath("process_log.csv"),
              "Category,Rank,Name,PID,CPU%,RAM_Bytes,RAM_MB\n"
          )
      ),
      alertFile(
          openCsvFile(
              deriveSiblingPath("alert_log.csv"),
              "Metric,Type,Severity,Value,Reference,Message\n"
          )
      )
{
}

std::ofstream Logger::openCsvFile(
    const std::filesystem::path& path,
    const std::string& header
) const
{
    const auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }

    std::ofstream file(path, std::ios::app);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open log file: " + path.string());
    }

    if (std::filesystem::exists(path) &&
        std::filesystem::file_size(path) == 0)
    {
        file << header;
    }

    return file;
}

std::filesystem::path Logger::deriveSiblingPath(
    const std::string& filename
) const
{
    return systemLogPath.parent_path() / filename;
}

void Logger::logSystem(
    double cpuPercent,
    double ramPercent,
    uint64_t ramUsedBytes
)
{
    systemFile << cpuPercent << ","
               << ramPercent << ","
               << ramUsedBytes << "\n";
}

void Logger::logProcesses(
    const std::vector<ProcessInfo>& topCpuProcesses,
    const std::vector<ProcessInfo>& topRamProcesses
)
{
    logProcessGroup("TOP_CPU", topCpuProcesses);
    logProcessGroup("TOP_RAM", topRamProcesses);
}

void Logger::logProcessGroup(
    const std::string& category,
    const std::vector<ProcessInfo>& processes
)
{
    for (size_t i = 0; i < processes.size(); ++i)
    {
        const auto& process = processes[i];
        double ramMb =
            static_cast<double>(process.ramBytes) / 1024.0 / 1024.0;

        processFile << category << ","
                    << (i + 1) << ","
                    << process.name << ","
                    << process.pid << ","
                    << process.cpuPercent << ","
                    << process.ramBytes << ","
                    << ramMb << "\n";
    }
}

void Logger::logAlert(const Alert& alert)
{
    alertFile << alert.metric << ","
              << alertTypeToString(alert.type) << ","
              << severityToString(alert.severity) << ","
              << alert.value << ","
              << alert.reference << ","
              << alert.message << "\n";
}
