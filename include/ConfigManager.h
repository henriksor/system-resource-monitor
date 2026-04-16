#pragma once

#include <string>

class ConfigManager {
public:
    explicit ConfigManager(const std::string& path);

    double cpuThreshold() const;
    double ramThreshold() const;

    double cpuSpikeThreshold() const;
    double ramSpikeThreshold() const;

    double cpuLeakThreshold() const;
    double ramLeakThresholdPercent() const;
    double ramLeakThresholdMb() const;

    size_t historySize() const;

    std::string logFile() const;

private:
    void load(const std::string& path);

    double m_cpuThreshold;
    double m_ramThreshold;

    double m_cpuSpikeThreshold;
    double m_ramSpikeThreshold;

    double m_cpuLeakThreshold;
    double m_ramLeakThresholdPercent;
    double m_ramLeakThresholdMb;

    size_t m_historySize;

    std::string m_logFile;
};
