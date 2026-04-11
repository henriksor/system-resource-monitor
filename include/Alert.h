#pragma once

#include <string>

enum class Severity {
    Info,
    Warning,
    Critical
};

enum class AlertType {
    Threshold,
    Spike,
    Leak
};

struct Alert {
    std::string metric;   // "CPU" or "RAM"
    AlertType type;
    Severity severity;
    double value;
    double reference;     // threshold / previous / average
    std::string message;
};