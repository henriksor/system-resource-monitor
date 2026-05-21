#include <catch2/catch.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <json.hpp>
#include <optional>
#include <stdexcept>
#include <string>

#include "AnomalyDetector.h"
#include "AlertHandler.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "SnapshotJson.h"

namespace {
std::filesystem::path makeTestRoot()
{
    const auto suffix =
        std::chrono::steady_clock::now().time_since_epoch().count();
    auto root = std::filesystem::temp_directory_path() /
                ("system-resource-monitor-tests-" + std::to_string(suffix));
    std::filesystem::create_directories(root);
    return root;
}

void writeConfig(
    const std::filesystem::path& path,
    const std::string& logFile = "logs/system_log.csv",
    const std::string& historySize = "5"
)
{
    std::ofstream file(path, std::ios::trunc);
    file << R"({
  "thresholds": {
    "cpu": 80,
    "ram": 80
  },
  "spike": {
    "cpu": 10,
    "ram": 10
  },
  "leak": {
    "cpu": 15,
    "ram": {
      "percent": 15,
      "mb": 15
    }
  },
  "historySize": )" << historySize << R"(,
  "logFile": ")" << logFile << R"("
})";
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}
} // namespace

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

TEST_CASE("Threshold breaches trigger warning alerts")
{
    AnomalyDetector detector(
        80.0,
        75.0,
        100.0,
        100.0,
        100.0,
        100.0,
        5
    );

    const auto alerts = detector.check(81.0, 76.0);

    REQUIRE(alerts.size() == 2);
    CHECK(alerts[0].metric == "CPU");
    CHECK(alerts[0].type == AlertType::Threshold);
    CHECK(alerts[0].severity == Severity::Warning);
    CHECK(alerts[1].metric == "RAM");
    CHECK(alerts[1].type == AlertType::Threshold);
    CHECK(alerts[1].severity == Severity::Warning);
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

TEST_CASE("Leak trend triggers a critical alert")
{
    AnomalyDetector detector(
        95.0,
        95.0,
        100.0,
        100.0,
        5.0,
        100.0,
        3
    );

    REQUIRE(detector.check(10.0, std::nullopt).empty());
    REQUIRE(detector.check(10.0, std::nullopt).empty());

    const auto alerts = detector.check(30.0, std::nullopt);

    REQUIRE(alerts.size() == 1);
    CHECK(alerts.front().metric == "CPU");
    CHECK(alerts.front().type == AlertType::Leak);
    CHECK(alerts.front().severity == Severity::Critical);
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

TEST_CASE("SystemSnapshot serializes missing values as null")
{
    SystemSnapshot snapshot;

    const nlohmann::json jsonSnapshot = snapshot;

    REQUIRE(jsonSnapshot["cpuPercent"].is_null());
    REQUIRE(jsonSnapshot["ramPercent"].is_null());
    REQUIRE(jsonSnapshot["memory"].is_null());
    REQUIRE(jsonSnapshot["processes"].empty());
    REQUIRE(jsonSnapshot["alerts"].empty());
}

TEST_CASE("ConfigManager validates required fields and ranges")
{
    auto root = makeTestRoot();
    const auto configPath = root / "config.json";
    writeConfig(configPath);

    ConfigManager config(configPath.string(), root);

    CHECK(config.cpuThreshold() == Approx(80.0));
    CHECK(config.historySize() == 5);
    CHECK(
        std::filesystem::path(config.logFile()).filename() ==
        "system_log.csv"
    );

    std::filesystem::remove_all(root);
}

TEST_CASE("ConfigManager rejects invalid history size")
{
    auto root = makeTestRoot();
    const auto configPath = root / "config.json";
    writeConfig(configPath, "logs/system_log.csv", "1");

    REQUIRE_THROWS_AS(ConfigManager(configPath.string(), root), std::runtime_error);

    std::filesystem::remove_all(root);
}

TEST_CASE("ConfigManager rejects log files outside logs directory")
{
    auto root = makeTestRoot();
    const auto configPath = root / "config.json";
    writeConfig(configPath, "../system_log.csv");

    REQUIRE_THROWS_AS(ConfigManager(configPath.string(), root), std::runtime_error);

    std::filesystem::remove_all(root);
}

TEST_CASE("Logger writes headers, escapes CSV fields, and appends")
{
    auto root = makeTestRoot();
    const auto logPath = root / "logs" / "system_log.csv";

    {
        Logger logger(logPath.string());
        logger.logSystem(10.0, 20.0, 1024);
        logger.logProcesses(
            {ProcessInfo{123, "proc \"name\"", 2048, 5.5}},
            {}
        );
        logger.logAlert(Alert{
            "CPU",
            AlertType::Spike,
            Severity::Warning,
            10.0,
            5.0,
            "quoted \"alert\""
        });
        logger.flush();
    }

    {
        Logger logger(logPath.string());
        logger.logSystem(11.0, 21.0, 2048);
        logger.flush();
    }

    const auto systemLog = readFile(logPath);
    const auto processLog = readFile(root / "logs" / "process_log.csv");
    const auto alertLog = readFile(root / "logs" / "alert_log.csv");

    REQUIRE(systemLog.find("CPU%,RAM%,RAM_Used_Bytes\n") == 0);
    REQUIRE(systemLog.find("CPU%,RAM%,RAM_Used_Bytes\n", 1) == std::string::npos);
    CHECK(processLog.find("\"proc \"\"name\"\"\"") != std::string::npos);
    CHECK(alertLog.find("\"quoted \"\"alert\"\"\"") != std::string::npos);

    std::filesystem::remove_all(root);
}

TEST_CASE("Logger reports open failures")
{
    auto root = makeTestRoot();
    const auto blockedPath = root / "logs" / "system_log.csv";
    std::filesystem::create_directories(blockedPath);

    REQUIRE_THROWS_AS(Logger(blockedPath.string()), std::runtime_error);

    std::filesystem::remove_all(root);
}

TEST_CASE("WebhookAlertHandler requires HTTPS URLs")
{
    REQUIRE_NOTHROW(WebhookAlertHandler("https://example.com/alerts"));
    REQUIRE_THROWS_AS(
        WebhookAlertHandler("http://example.com/alerts"),
        std::runtime_error
    );
}
