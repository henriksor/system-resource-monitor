# System Resource Monitor

`SystemResourceMonitor` is a Windows-based C++17 monitoring tool. It samples
system CPU and memory usage, collects process-level CPU/RAM data, detects
system and process anomalies, writes CSV logs, and publishes the latest snapshot
as JSON.

## Features

- Live system CPU and RAM monitoring
- Process table output for top CPU/RAM consumers
- System anomaly detection for thresholds, spikes, and leak/trend behavior
- Process anomaly detection for CPU spikes and RAM growth patterns
- CSV logs:
  - `logs/system_log.csv`
  - `logs/process_log.csv`
  - `logs/alert_log.csv`
- JSON snapshot:
  - `logs/latest_snapshot.json`
- Optional external alert delivery through webhook or mock email handlers
- Single-instance runtime guard on Windows
- Graceful shutdown with `Ctrl+C`

## Build

Requirements:

- Windows
- CMake 3.16+
- A C++17-capable compiler

Default local builds do not build tests. This avoids fetching test dependencies
unless they are explicitly requested.

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Run the monitor:

```powershell
.\build\Debug\SystemResourceMonitor.exe
```

Stop it with `Ctrl+C`.

## Tests

Unit tests use Catch2. `BUILD_TESTING` is intentionally `OFF` by default, but CI
enables it explicitly.

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

GitHub Actions runs the same Debug configuration on Windows.

## Configuration

Runtime configuration lives in `config.json`.

Supported fields:

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

`logFile` must resolve to a file under the runtime `logs/` directory. The logger
derives `process_log.csv` and `alert_log.csv` beside that system log.

## Alerts

Console alerts are always enabled. External alert handlers are enabled by
environment variables:

- `SYSTEM_MONITOR_WEBHOOK_URL`: HTTPS webhook endpoint for JSON alert payloads
- `SYSTEM_MONITOR_EMAIL_TO`: mock email recipient printed to stderr

Webhook URLs must use HTTPS. Invalid webhook configuration fails at startup.
External alert delivery is non-blocking for the sampling loop; alerts can be
dropped locally if the bounded queue is full.

## Robustness Contracts

- Missing CPU or RAM measurements are represented with `std::optional`.
- Missing measurements are not treated as `0`.
- Console output shows missing measurements as `N/A`.
- JSON output writes missing measurements as `null`.
- Anomaly detection does not alert on missing measurements.
- CSV writes are checked and flushed after each snapshot.
- Only one monitor instance is supported; a second instance exits with an error.
- The latest JSON snapshot is written to `logs/latest_snapshot.json`.

## Known Constraints

- Windows-only implementation.
- The email handler is a mock transport.
- Multiple concurrent monitor instances are intentionally blocked.
- JSON snapshot schema and CSV column layout are kept stable for this version.
