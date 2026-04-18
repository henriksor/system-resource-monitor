#pragma once

#include <json.hpp>
#include <optional>

#include "Alert.h"
#include "ProcessInfo.h"
#include "SystemSnapshot.h"

inline const char* severityToString(Severity severity)
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

inline const char* alertTypeToString(AlertType type)
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

inline void to_json(nlohmann::json& j, const ProcessInfo& process)
{
    j = nlohmann::json{
        {"pid", process.pid},
        {"name", process.name},
        {"ramBytes", process.ramBytes},
        {"cpuPercent", process.cpuPercent}
    };
}

inline void to_json(nlohmann::json& j, const Alert& alert)
{
    j = nlohmann::json{
        {"metric", alert.metric},
        {"type", alertTypeToString(alert.type)},
        {"severity", severityToString(alert.severity)},
        {"value", alert.value},
        {"reference", alert.reference},
        {"message", alert.message}
    };
}

inline void to_json(
    nlohmann::json& j,
    const SystemSnapshot::MemoryDetails& memoryDetails
)
{
    j = nlohmann::json{
        {"usedBytes", memoryDetails.usedBytes},
        {"totalBytes", memoryDetails.totalBytes},
        {"availableBytes", memoryDetails.availableBytes}
    };
}

inline void to_json(nlohmann::json& j, const SystemSnapshot& snapshot)
{
    j = nlohmann::json{
        {"cpuPercent",
         snapshot.cpuPercent.has_value()
             ? nlohmann::json(*snapshot.cpuPercent)
             : nlohmann::json(nullptr)},
        {"ramPercent",
         snapshot.ramPercent.has_value()
             ? nlohmann::json(*snapshot.ramPercent)
             : nlohmann::json(nullptr)},
        {"memory",
         snapshot.memory.has_value()
             ? nlohmann::json(*snapshot.memory)
             : nlohmann::json(nullptr)},
        {"processes", snapshot.processes},
        {"alerts", snapshot.alerts}
    };
}
