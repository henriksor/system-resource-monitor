#pragma once

#include <vector>
#include <string>

class AnomalyDetector {
public:
    AnomalyDetector(double cpuThreshold, double ramThreshold, double cpuSpikeThreshold, double ramSpikeThreshold, double cpuLeakThreshold, double ramLeakThreshold, size_t historySize = 5);

    std::vector<std::string> check(double cpuPercent, double ramPercent);

private:
    const double cpuThreshold;
    const double ramThreshold;

    const double cpuSpikeThreshold;
    const double ramSpikeThreshold;

    const double cpuLeakThreshold;
    const double ramLeakThreshold;

    const size_t maxHistory;

    std::vector<double> cpuHistory;
    std::vector<double> ramHistory;

    double calculateAverage(const std::vector<double>& values) const;

    bool isMonotonicIncreasing(const std::vector<double>& values) const;
};