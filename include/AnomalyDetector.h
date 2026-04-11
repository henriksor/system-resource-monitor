#pragma once

#include <vector>
#include "Alert.h"

class AnomalyDetector {
public:
    AnomalyDetector(
        double cpuThreshold,
        double ramThreshold,
        double cpuSpikeThreshold,
        double ramSpikeThreshold,
        double cpuLeakThreshold,
        double ramLeakThreshold,
        size_t historySize
    );

    std::vector<Alert> check(double cpuPercent, double ramPercent);

private:
    void addToHistory(double cpuPercent, double ramPercent);

    std::vector<Alert> checkThreshold(double cpuPercent, double ramPercent);
    std::vector<Alert> checkSpike(double cpuPercent, double ramPercent);
    std::vector<Alert> checkLeak(double cpuPercent, double ramPercent);

    double calculateAverage(const std::vector<double>& values) const;

    double cpuThreshold;
    double ramThreshold;
    double cpuSpikeThreshold;
    double ramSpikeThreshold;
    double cpuLeakThreshold;
    double ramLeakThreshold;

    size_t maxHistory;

    std::vector<double> cpuHistory;
    std::vector<double> ramHistory;
};