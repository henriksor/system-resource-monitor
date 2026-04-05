#include "ConfigManager.h"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
    file >> j;

    m_cpuThreshold      = j["thresholds"]["cpu"];
    m_ramThreshold      = j["thresholds"]["ram"];

    m_cpuSpikeThreshold = j["spike"]["cpu"];
    m_ramSpikeThreshold = j["spike"]["ram"];

    m_cpuLeakThreshold  = j["leak"]["cpu"];
    m_ramLeakThreshold  = j["leak"]["ram"];

    m_historySize       = j["historySize"];

    m_logFile           = j["logFile"];
}

double ConfigManager::cpuThreshold() const { return m_cpuThreshold; }
double ConfigManager::ramThreshold() const { return m_ramThreshold; }

double ConfigManager::cpuSpikeThreshold() const { return m_cpuSpikeThreshold; }
double ConfigManager::ramSpikeThreshold() const { return m_ramSpikeThreshold; }

double ConfigManager::cpuLeakThreshold() const { return m_cpuLeakThreshold; }
double ConfigManager::ramLeakThreshold() const { return m_ramLeakThreshold; }

size_t ConfigManager::historySize() const { return m_historySize; }

std::string ConfigManager::logFile() const { return m_logFile; }