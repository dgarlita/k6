// ===============================
// GANTI IP INI SESUAI IP ESP32 LU
// ===============================
const ESP32_URL = "http://192.168.100.46";
// ===============================

let soilValue = 0;
let soilRaw = 0;
let tempValue = 0;
let humValue = 0;

let relay1State = false;
let relay2State = false;

let tempData = [];
let humData = [];
let maxData = 20;

const soilGauge = document.getElementById("soilGauge");
const soilCtx = soilGauge.getContext("2d");

const lineChart = document.getElementById("lineChart");
const lineCtx = lineChart.getContext("2d");

function updateDisplay() {
  document.getElementById("soilValue").innerText = soilValue;
  document.getElementById("soilRaw").innerText = soilRaw;
  document.getElementById("tempValue").innerText = tempValue.toFixed(1);
  document.getElementById("humValue").innerText = humValue.toFixed(1);

  drawGauge(soilValue);
  drawLineChart();
  updateRelayButtons();
}

function drawGauge(value) {
  soilCtx.clearRect(0, 0, soilGauge.width, soilGauge.height);

  let centerX = soilGauge.width / 2;
  let centerY = 140;
  let radius = 95;

  soilCtx.beginPath();
  soilCtx.arc(centerX, centerY, radius, Math.PI, 2 * Math.PI);
  soilCtx.lineWidth = 22;
  soilCtx.strokeStyle = "#ddd";
  soilCtx.stroke();

  let endAngle = Math.PI + (value / 100) * Math.PI;

  soilCtx.beginPath();
  soilCtx.arc(centerX, centerY, radius, Math.PI, endAngle);
  soilCtx.lineWidth = 22;

  if (value < 30) {
    soilCtx.strokeStyle = "#d9534f";
  } else if (value < 60) {
    soilCtx.strokeStyle = "#f0ad4e";
  } else {
    soilCtx.strokeStyle = "#1f6f43";
  }

  soilCtx.stroke();

  soilCtx.beginPath();
  soilCtx.moveTo(centerX, centerY);

  let needleX = centerX + Math.cos(endAngle) * 75;
  let needleY = centerY + Math.sin(endAngle) * 75;

  soilCtx.lineTo(needleX, needleY);
  soilCtx.lineWidth = 4;
  soilCtx.strokeStyle = "#222";
  soilCtx.stroke();

  soilCtx.beginPath();
  soilCtx.arc(centerX, centerY, 7, 0, 2 * Math.PI);
  soilCtx.fillStyle = "#222";
  soilCtx.fill();

  soilCtx.font = "14px Arial";
  soilCtx.fillStyle = "#555";
  soilCtx.fillText("0%", 28, 145);
  soilCtx.fillText("100%", 205, 145);
}

function drawLineChart() {
  lineCtx.clearRect(0, 0, lineChart.width, lineChart.height);

  let width = lineChart.width;
  let height = lineChart.height;
  let padding = 35;

  lineCtx.strokeStyle = "#ccc";
  lineCtx.lineWidth = 1;

  lineCtx.beginPath();
  lineCtx.moveTo(padding, padding);
  lineCtx.lineTo(padding, height - padding);
  lineCtx.lineTo(width - padding, height - padding);
  lineCtx.stroke();

  lineCtx.font = "12px Arial";
  lineCtx.fillStyle = "#555";
  lineCtx.fillText("100", 5, padding + 5);
  lineCtx.fillText("50", 12, height / 2);
  lineCtx.fillText("0", 20, height - padding);

  drawDataLine(tempData, "#d9534f");
  drawDataLine(humData, "#1f6f43");

  lineCtx.fillStyle = "#d9534f";
  lineCtx.fillText("Suhu (C)", width - 130, 20);

  lineCtx.fillStyle = "#1f6f43";
  lineCtx.fillText("Kelembapan (%)", width - 130, 40);
}

function drawDataLine(dataArray, color) {
  if (dataArray.length < 2) {
    return;
  }

  let width = lineChart.width;
  let height = lineChart.height;

  let padding = 35;
  let chartWidth = width - padding * 2;
  let chartHeight = height - padding * 2;

  lineCtx.beginPath();

  for (let i = 0; i < dataArray.length; i++) {
    let x = padding + (i / (maxData - 1)) * chartWidth;
    let y = height - padding - (dataArray[i] / 100) * chartHeight;

    if (i === 0) {
      lineCtx.moveTo(x, y);
    } else {
      lineCtx.lineTo(x, y);
    }
  }

  lineCtx.strokeStyle = color;
  lineCtx.lineWidth = 3;
  lineCtx.stroke();
}

function updateRelayButtons() {
  let relay1Btn = document.getElementById("relay1Btn");
  let relay2Btn = document.getElementById("relay2Btn");

  if (relay1State) {
    relay1Btn.innerText = "Relay 1 / Pompa: ON";
    relay1Btn.className = "on";
  } else {
    relay1Btn.innerText = "Relay 1 / Pompa: OFF";
    relay1Btn.className = "off";
  }

  if (relay2State) {
    relay2Btn.innerText = "Relay 2 / Aktuator: ON";
    relay2Btn.className = "on";
  } else {
    relay2Btn.innerText = "Relay 2 / Aktuator: OFF";
    relay2Btn.className = "off";
  }
}

function toggleRelay(channel) {
  let newState;

  if (channel === 1) {
    newState = relay1State ? 0 : 1;
  } else {
    newState = relay2State ? 0 : 1;
  }

  fetch(`${ESP32_URL}/relay?ch=${channel}&state=${newState}`)
    .then(response => {
      if (!response.ok) {
        throw new Error("HTTP error " + response.status);
      }
      return response.text();
    })
    .then(data => {
      console.log("Response relay:", data);

      if (channel === 1) {
        relay1State = newState === 1;
      } else {
        relay2State = newState === 1;
      }

      updateRelayButtons();
    })
    .catch(error => {
      console.log("Gagal mengirim perintah relay:", error);
      alert("Gagal mengirim perintah ke ESP32. Cek IP ESP32, jaringan, dan endpoint /relay.");
    });
}

function getSensorData() {
  fetch(`${ESP32_URL}/data`, {
    method: "GET",
    cache: "no-store"
  })
    .then(response => {
      if (!response.ok) {
        throw new Error("HTTP error " + response.status);
      }
      return response.json();
    })
    .then(data => {
      console.log("Data dari ESP32:", data);

      document.getElementById("status").innerText = "Status: Terhubung ke ESP32";
      document.getElementById("status").className = "status online";

      soilValue = Number(data.soil) || 0;
      soilRaw = Number(data.soilRaw) || 0;
      tempValue = Number(data.temperature) || 0;
      humValue = Number(data.humidity) || 0;

      relay1State = Number(data.relay1) === 1;
      relay2State = Number(data.relay2) === 1;

      tempData.push(tempValue);
      humData.push(humValue);

      if (tempData.length > maxData) {
        tempData.shift();
      }

      if (humData.length > maxData) {
        humData.shift();
      }

      updateDisplay();
    })
    .catch(error => {
      console.log("Gagal mengambil data sensor:", error);

      document.getElementById("status").innerText = "Status: Tidak terhubung ke ESP32";
      document.getElementById("status").className = "status offline";
    });
}

setInterval(getSensorData, 3000);

drawGauge(0);
drawLineChart();
updateRelayButtons();
getSensorData();