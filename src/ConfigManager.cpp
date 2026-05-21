#include "ConfigManager.h"
#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <json.hpp>

using json = nlohmann::json;

namespace {
const json& requireObject(const json& parent, const char* key)
{
    auto it = parent.find(key);
    if (it == parent.end() || !it->is_object())
    {
        throw std::runtime_error(std::string("Config field '") + key +
                                 "' must be an object");
    }

    return *it;
}

double requireNumberInRange(
    const json& parent,
    const char* key,
    double minValue,
    double maxValue
)
{
    auto it = parent.find(key);
    if (it == parent.end() || !it->is_number())
    {
        throw std::runtime_error(std::string("Config field '") + key +
                                 "' must be a number");
    }

    double value = it->get<double>();
    if (value < minValue || value > maxValue)
    {
        std::ostringstream message;
        message << "Config field '" << key << "' must be between "
                << minValue << " and " << maxValue;
        throw std::runtime_error(message.str());
    }

    return value;
}

size_t requireHistorySize(const json& parent, const char* key)
{
    auto it = parent.find(key);
    if (it == parent.end() || !it->is_number_integer())
    {
        throw std::runtime_error(
            std::string("Config field '") + key +
            "' must be an integer greater than or equal to 2"
        );
    }

    int64_t value = it->get<int64_t>();
    if (value < 2)
    {
        throw std::runtime_error(
            std::string("Config field '") + key +
            "' must be greater than or equal to 2"
        );
    }

    if (static_cast<uint64_t>(value) > std::numeric_limits<size_t>::max())
    {
        throw std::runtime_error(
            std::string("Config field '") + key + "' is too large"
        );
    }

    return static_cast<size_t>(value);
}

std::string requireString(const json& parent, const char* key)
{
    auto it = parent.find(key);
    if (it == parent.end() || !it->is_string())
    {
        throw std::runtime_error(std::string("Config field '") + key +
                                 "' must be a string");
    }

    std::string value = it->get<std::string>();
    if (value.empty())
    {
        throw std::runtime_error(std::string("Config field '") + key +
                                 "' must not be empty");
    }

    return value;
}

bool pathStartsWith(
    const std::filesystem::path& path,
    const std::filesystem::path& base
)
{
    auto pathIt = path.begin();
    auto baseIt = base.begin();

    for (; baseIt != base.end(); ++baseIt, ++pathIt)
    {
        if (pathIt == path.end() || *pathIt != *baseIt)
        {
            return false;
        }
    }

    return true;
}
} // namespace

ConfigManager::ConfigManager(const std::string& path)
{
    load(path);
    validateLogFilePath(std::filesystem::path(path).parent_path());
}

ConfigManager::ConfigManager(
    const std::string& path,
    const std::filesystem::path& runtimeRoot
)
{
    load(path);
    validateLogFilePath(runtimeRoot);
}

void ConfigManager::load(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        throw std::runtime_error("Could not open config file");

    json j;
    try
    {
        file >> j;
    }
    catch (const json::exception& ex)
    {
        throw std::runtime_error(
            std::string("Invalid JSON in config file: ") + ex.what()
        );
    }

    const auto& thresholds = requireObject(j, "thresholds");
    const auto& spike = requireObject(j, "spike");
    const auto& leak = requireObject(j, "leak");
    const auto& ramLeak = requireObject(leak, "ram");

    // These thresholds are percentages used in anomaly rules, so enforcing a
    // sane range at the boundary avoids surprising runtime behavior later on.
    m_cpuThreshold      = requireNumberInRange(thresholds, "cpu", 0.0, 100.0);
    m_ramThreshold      = requireNumberInRange(thresholds, "ram", 0.0, 100.0);

    m_cpuSpikeThreshold = requireNumberInRange(spike, "cpu", 0.0, 100.0);
    m_ramSpikeThreshold = requireNumberInRange(spike, "ram", 0.0, 100.0);

    m_cpuLeakThreshold        = requireNumberInRange(leak, "cpu", 0.0, 100.0);
    m_ramLeakThresholdPercent = requireNumberInRange(
        ramLeak,
        "percent",
        0.0,
        100.0
    );
    m_ramLeakThresholdMb      = requireNumberInRange(
        ramLeak,
        "mb",
        0.0,
        std::numeric_limits<double>::max()
    );

    m_historySize       = requireHistorySize(j, "historySize");

    m_logFile           = requireString(j, "logFile");
}

void ConfigManager::validateLogFilePath(
    const std::filesystem::path& runtimeRoot
)
{
    const auto root =
        runtimeRoot.empty()
            ? std::filesystem::current_path()
            : std::filesystem::absolute(runtimeRoot);
    const auto logsRoot =
        std::filesystem::weakly_canonical(root / "logs");

    const auto configuredPath = std::filesystem::path(m_logFile);
    const auto candidate =
        configuredPath.is_absolute()
            ? configuredPath
            : root / configuredPath;
    const auto normalizedCandidate =
        std::filesystem::weakly_canonical(candidate);

    // Keep configured logging inside the runtime log directory even when the
    // config uses absolute paths or parent-directory traversal.
    if (!normalizedCandidate.has_filename() ||
        normalizedCandidate == logsRoot ||
        !pathStartsWith(normalizedCandidate, logsRoot))
    {
        throw std::runtime_error(
            "Config field 'logFile' must resolve to a file under the logs "
            "directory"
        );
    }

    m_logFile = normalizedCandidate.string();
}

double ConfigManager::cpuThreshold() const { return m_cpuThreshold; }
double ConfigManager::ramThreshold() const { return m_ramThreshold; }

double ConfigManager::cpuSpikeThreshold() const { return m_cpuSpikeThreshold; }
double ConfigManager::ramSpikeThreshold() const { return m_ramSpikeThreshold; }

double ConfigManager::cpuLeakThreshold() const { return m_cpuLeakThreshold; }
double ConfigManager::ramLeakThresholdPercent() const
{
    return m_ramLeakThresholdPercent;
}

double ConfigManager::ramLeakThresholdMb() const
{
    return m_ramLeakThresholdMb;
}

size_t ConfigManager::historySize() const { return m_historySize; }

std::string ConfigManager::logFile() const { return m_logFile; }
