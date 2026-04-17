#include "ConfigManager.h"
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
} // namespace

ConfigManager::ConfigManager(const std::string& path)
{
    load(path);
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
