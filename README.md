# System Resource Monitor

`SystemResourceMonitor` is a Windows-based C++ monitoring tool that tracks
system CPU and memory usage, surfaces the top CPU and memory-consuming
processes, detects both system-level and per-process anomalies, and publishes
the latest monitoring snapshot to multiple output targets.

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
- JSON snapshot publishing for a lightweight web dashboard:
  - `dashboard/latest_snapshot.json`
- Extensible output pipeline via `OutputHandler`
- Extensible external alert delivery via `AlertHandler`
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

### `dashboard/latest_snapshot.json`

The latest full `SystemSnapshot` serialized as JSON for the dashboard frontend:

- CPU percentage
- RAM percentage
- Memory details
- Process list
- Alert list

## Configuration

Runtime configuration lives in [config.json](C:/Code/projects/system-resource-monitor/config.json).

Current configuration fields:

- `thresholds.cpu`
- `thresholds.ram`
- `spike.cpu`
- `spike.ram`
- `leak.cpu`
- `leak.ram.percent`
- `leak.ram.mb`
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
    "ram": {
      "percent": 15,
      "mb": 15
    }
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

### Dashboard

The project includes a static dashboard in `dashboard/` that reads
`dashboard/latest_snapshot.json`.

Serve the repository root with a lightweight static server:

```powershell
python -m http.server
```

Then open:

```text
http://localhost:8000/dashboard/
```

## Project Structure

- `src/` application source files
- `include/` headers
- `dashboard/` static web dashboard assets
- `tests/` test-related files
- `logs/` generated runtime logs
- `external/json/` bundled JSON dependency
- `config.json` runtime configuration

## Implementation Overview

- `SystemMonitor` orchestrates the main monitoring loop and builds a
  `SystemSnapshot` each refresh.
- `CpuMonitor` and `MemoryMonitor` collect system-wide metrics.
- `ProcessMonitor` collects process snapshots and process-specific anomalies.
- `AnomalyDetector` handles global CPU/RAM anomaly detection only.
- `OutputHandler` implementations fan snapshots out to console, CSV, and JSON
  outputs.
- `AlertHandler` implementations deliver warning/critical alerts externally,
  such as webhooks or mock email notifications.
- `Logger` writes system metrics, process snapshots, and alerts to separate CSV
  files for the CSV output handler.
- `ConfigManager` loads thresholds and history settings from `config.json`.

## Status

The project currently compiles successfully in the existing workspace build
setup and reflects the current implementation, including:

- split CSV logging
- process table output
- JSON dashboard snapshot publishing
- output handler architecture
- alert handler integration
- per-process CPU spike detection
- per-process RAM leak detection
