function formatPercent(value) {
  if (value == null) {
    return "N/A";
  }
  return `${Number(value).toFixed(2)} %`;
}

function formatGigabytes(bytes) {
  if (bytes == null) {
    return "N/A";
  }
  return `${(Number(bytes) / 1024 / 1024 / 1024).toFixed(2)} GB`;
}

function formatMegabytes(bytes) {
  return `${(Number(bytes) / 1024 / 1024).toFixed(2)}`;
}

function renderSummary(snapshot) {
  const memory = snapshot.memory ?? {};

  document.getElementById("cpuValue").textContent =
    formatPercent(snapshot.cpuPercent);
  document.getElementById("ramValue").textContent =
    formatPercent(snapshot.ramPercent);
  document.getElementById("memoryUsed").textContent =
    formatGigabytes(memory.usedBytes);
  document.getElementById("memoryAvailable").textContent =
    formatGigabytes(memory.availableBytes);
  document.getElementById("memoryTotal").textContent =
    formatGigabytes(memory.totalBytes);
  document.getElementById("statusText").textContent =
    `Updated ${new Date().toLocaleTimeString()}`;
}

function renderProcesses(snapshot) {
  const processTableBody = document.getElementById("processTableBody");
  const topProcesses = [...snapshot.processes]
    .sort((left, right) => right.cpuPercent - left.cpuPercent)
    .slice(0, 10);

  processTableBody.replaceChildren();

  if (topProcesses.length === 0) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 4;
    cell.className = "empty-state";
    cell.textContent = "No process data available.";
    row.appendChild(cell);
    processTableBody.appendChild(row);
    return;
  }

  for (const process of topProcesses) {
    const row = document.createElement("tr");

    const nameCell = document.createElement("td");
    nameCell.textContent = process.name;
    row.appendChild(nameCell);

    const pidCell = document.createElement("td");
    pidCell.textContent = String(process.pid);
    row.appendChild(pidCell);

    const cpuCell = document.createElement("td");
    cpuCell.textContent = Number(process.cpuPercent).toFixed(2);
    row.appendChild(cpuCell);

    const ramCell = document.createElement("td");
    ramCell.textContent = formatMegabytes(process.ramBytes);
    row.appendChild(ramCell);

    processTableBody.appendChild(row);
  }
}

function renderAlerts(snapshot) {
  const alertList = document.getElementById("alertList");

  alertList.replaceChildren();

  if (!snapshot.alerts || snapshot.alerts.length === 0) {
    const item = document.createElement("li");
    item.className = "empty-state";
    item.textContent = "No alerts in latest snapshot.";
    alertList.appendChild(item);
    return;
  }

  for (const alert of snapshot.alerts) {
    const item = document.createElement("li");
    item.className = `alert-item alert-${alert.severity.toLowerCase()}`;

    const header = document.createElement("div");
    header.className = "alert-header";

    const severity = document.createElement("span");
    severity.className = "alert-severity";
    severity.textContent = alert.severity;
    header.appendChild(severity);

    const metric = document.createElement("span");
    metric.className = "alert-metric";
    metric.textContent = alert.metric;
    header.appendChild(metric);

    const message = document.createElement("p");
    message.textContent = alert.message;

    const meta = document.createElement("p");
    meta.className = "alert-meta";
    meta.textContent =
      `Type: ${alert.type} | Value: ${Number(alert.value).toFixed(2)}`;

    item.appendChild(header);
    item.appendChild(message);
    item.appendChild(meta);
    alertList.appendChild(item);
  }
}

async function loadSnapshot() {
  try {
    const response = await fetch(`latest_snapshot.json?ts=${Date.now()}`);
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    const snapshot = await response.json();
    renderSummary(snapshot);
    renderProcesses(snapshot);
    renderAlerts(snapshot);
  } catch (error) {
    document.getElementById("statusText").textContent =
      `Waiting for data (${error.message})`;
  }
}

loadSnapshot();
setInterval(loadSnapshot, 2000);
