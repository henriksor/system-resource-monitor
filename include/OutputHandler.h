#pragma once

#include <filesystem>
#include <memory>

#include "Logger.h"
#include "SystemSnapshot.h"

class OutputHandler {
public:
    virtual ~OutputHandler() = default;
    virtual void handle(const SystemSnapshot& snapshot) = 0;
};

class ConsoleOutputHandler : public OutputHandler {
public:
    void handle(const SystemSnapshot& snapshot) override;
};

class CsvLoggerOutputHandler : public OutputHandler {
public:
    explicit CsvLoggerOutputHandler(const std::string& filename);

    void handle(const SystemSnapshot& snapshot) override;

private:
    Logger logger;
};

class JsonSnapshotOutputHandler : public OutputHandler {
public:
    explicit JsonSnapshotOutputHandler(std::filesystem::path outputPath);

    void handle(const SystemSnapshot& snapshot) override;

private:
    std::filesystem::path outputPath;
};
