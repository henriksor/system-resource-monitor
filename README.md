# System Resource Monitor

`SystemResourceMonitor` is a Windows-based C++ monitoring tool that tracks
system CPU and memory usage, surfaces the top memory and CPU-consuming
processes, and detects both system-level and per-process anomalies.

The current version focuses on a simple terminal dashboard plus CSV logging for
system metrics, process snapshots, and alerts.

## Features

- Live system monitoring for overall CPU and RAM usage
- Per-refresh process table output for:
  - Top 5 CPU consumers
  - Top 5 RAM consumers
  - Other processes above 1% CPU or RAM
- System-wide anomaly detection for:
  - Threshold breaches
  - Sudden spikes
  - Leak/trend behavior
- Per-process anomaly detection for:
  - CPU spikes
  - RAM leak-style growth across recent samples
- CSV logging split by concern:
  - `logs/system_log.csv`
  - `logs/process_log.csv`
  - `logs/alert_log.csv`
- Graceful shutdown with `Ctrl+C`

## Current Behavior

The monitor refreshes continuously and prints:

- Current system CPU usage
- Current system memory usage
- A formatted process table with columns:
  - `Name`
  - `PID`
  - `CPU%`
  - `RAM MB`
- Colored alerts in the console:
  - Blue for info
  - Yellow for warnings
  - Red for critical alerts

### Process monitoring notes

- Process CPU usage is calculated using deltas between samples.
- Per-process CPU history is preserved inside `ProcessMonitor`.
- CPU timing is only collected for processes using at least `0.1%` of total
  physical RAM. This reduces the cost of calling `GetProcessTimes()` for very
  small processes.
- Per-process history is cleaned up automatically when a PID disappears, so the
  internal tracking maps do not grow indefinitely.

## Alert Types

### System alerts

System alerts are handled by the global anomaly detector and can flag:

- CPU threshold breaches
- RAM threshold breaches
- CPU spikes
- RAM spikes
- CPU leak/trend behavior
- RAM leak/trend behavior

### Process alerts

Process alerts are handled inside `ProcessMonitor` and can flag:

- CPU spikes for individual processes
- RAM leak patterns for individual processes when:
  - RAM increases consistently across the configured history window
  - Average growth exceeds the configured leak threshold

Process-specific alerts include the PID, process name, metric type, and value
inside the alert message while keeping the CSV alert schema consistent.

## Logging

The logger now writes to three separate files:

### `logs/system_log.csv`

System-wide metrics for each refresh:

- CPU%
- RAM%
- RAM used bytes

### `logs/process_log.csv`

Process snapshots for each refresh:

- Category (`TOP_CPU` or `TOP_RAM`)
- Rank
- Process name
- PID
- CPU%
- RAM bytes
- RAM MB

### `logs/alert_log.csv`

All alerts with a consistent schema:

- Metric
- Type
- Severity
- Value
- Reference
- Message

## Configuration

Runtime configuration lives in [config.json](C:/Code/projects/system-resource-monitor/config.json).

Current configuration fields:

- `thresholds.cpu`
- `thresholds.ram`
- `spike.cpu`
- `spike.ram`
- `leak.cpu`
- `leak.ram`
- `historySize`
- `logFile`

Example:

```json
{
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
    "ram": 15
  },
  "historySize": 5,
  "logFile": "logs/system_log.csv"
}
```

## Build

Requirements:

- Windows
- CMake 3.16+
- A C++17-capable compiler

Build commands:

```powershell
cmake -S . -B build
cmake --build build
```

## Run

After building:

```powershell
.\build\SystemResourceMonitor.exe
```

Stop the monitor with `Ctrl+C`.

## Project Structure

- `src/` application source files
- `include/` headers
- `tests/` test-related files
- `logs/` generated runtime logs
- `external/json/` bundled JSON dependency
- `config.json` runtime configuration

## Implementation Overview

- `SystemMonitor` orchestrates the main monitoring loop, console output, and
  alert/log integration.
- `CpuMonitor` and `MemoryMonitor` collect system-wide metrics.
- `ProcessMonitor` collects process snapshots and process-specific anomalies.
- `AnomalyDetector` handles global CPU/RAM anomaly detection only.
- `Logger` writes system metrics, process snapshots, and alerts to separate CSV
  files.
- `ConfigManager` loads thresholds and history settings from `config.json`.

## Status

The project currently compiles successfully in the existing workspace build
setup and reflects the current implementation, including:

- split CSV logging
- process table output
- per-process CPU spike detection
- per-process RAM leak detection
