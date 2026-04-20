#include <catch2/catch.hpp>

#include <json.hpp>
#include <optional>

#include "AnomalyDetector.h"
#include "SnapshotJson.h"

TEST_CASE("AnomalyDetector handles std::nullopt without producing alerts")
{
    AnomalyDetector detector(
        80.0,
        80.0,
        20.0,
        20.0,
        10.0,
        10.0,
        5
    );

    const auto alerts = detector.check(std::nullopt, std::nullopt);

    REQUIRE(alerts.empty());
}

TEST_CASE("CPU spike triggers an alert")
{
    AnomalyDetector detector(
        95.0,
        95.0,
        20.0,
        20.0,
        50.0,
        50.0,
        5
    );

    REQUIRE(detector.check(10.0, std::nullopt).empty());

    const auto alerts = detector.check(45.0, std::nullopt);

    REQUIRE(alerts.size() == 1);
    CHECK(alerts.front().metric == "CPU");
    CHECK(alerts.front().type == AlertType::Spike);
    CHECK(alerts.front().severity == Severity::Warning);
    CHECK(alerts.front().value == Approx(45.0));
    CHECK(alerts.front().reference == Approx(35.0));
}

TEST_CASE("SystemSnapshot serializes correctly to JSON")
{
    SystemSnapshot snapshot;
    snapshot.cpuPercent = 42.5;
    snapshot.ramPercent = 68.0;
    snapshot.memory = SystemSnapshot::MemoryDetails{
        4ULL * 1024 * 1024 * 1024,
        8ULL * 1024 * 1024 * 1024,
        4ULL * 1024 * 1024 * 1024
    };
    snapshot.processes.push_back(ProcessInfo{1234, "monitor.exe", 1024, 12.5});
    snapshot.alerts.push_back(Alert{
        "CPU",
        AlertType::Spike,
        Severity::Warning,
        42.5,
        20.0,
        "CPU spike detected"
    });

    const nlohmann::json jsonSnapshot = snapshot;

    REQUIRE(jsonSnapshot.contains("cpuPercent"));
    REQUIRE(jsonSnapshot.contains("ramPercent"));
    REQUIRE(jsonSnapshot.contains("memory"));
    REQUIRE(jsonSnapshot.contains("processes"));
    REQUIRE(jsonSnapshot.contains("alerts"));

    REQUIRE(jsonSnapshot["cpuPercent"].get<double>() == Approx(42.5));
    REQUIRE(jsonSnapshot["ramPercent"].get<double>() == Approx(68.0));
    REQUIRE(jsonSnapshot["memory"]["usedBytes"] == 4ULL * 1024 * 1024 * 1024);
    REQUIRE(jsonSnapshot["processes"].size() == 1);
    REQUIRE(jsonSnapshot["processes"][0]["name"] == "monitor.exe");
    REQUIRE(jsonSnapshot["alerts"].size() == 1);
    REQUIRE(jsonSnapshot["alerts"][0]["severity"] == "Warning");
    REQUIRE(jsonSnapshot["alerts"][0]["type"] == "Spike");
}
