#pragma once

#include <optional>
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

    std::vector<Alert> check(
        const std::optional<double>& cpuPercent,
        const std::optional<double>& ramPercent
    );

private:
    void addToHistory(
        const std::optional<double>& cpuPercent,
        const std::optional<double>& ramPercent
    );

    std::vector<Alert> checkThreshold(
        const std::optional<double>& cpuPercent,
        const std::optional<double>& ramPercent
    );
    std::vector<Alert> checkSpike(
        const std::optional<double>& cpuPercent,
        const std::optional<double>& ramPercent
    );
    std::vector<Alert> checkLeak(
        const std::optional<double>& cpuPercent,
        const std::optional<double>& ramPercent
    );

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
