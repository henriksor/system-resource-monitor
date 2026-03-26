#pragma once
#include <string>
#include <vector>

class AnomalyDetector {
public:
    AnomalyDetector(double cpuThreshold, double ramThreshold, double cpuSpikeThreshold, double ramSpikeThreshold);
    std::vector<std::string> check(double cpuPercent, double ramPercent);

private:
    double cpuThreshold;
    double ramThreshold;

    double cpuSpikeThreshold;
    double ramSpikeThreshold;

    double previousCpu = -1.0;
    double previousRam = -1.0;
};