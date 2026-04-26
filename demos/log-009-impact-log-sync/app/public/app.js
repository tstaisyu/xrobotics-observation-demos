const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
const ws = new WebSocket(`${protocol}://${window.location.host}`);

const countEl = document.getElementById('count');
const levelEl = document.getElementById('level');
const deltaEl = document.getElementById('delta');
const logsEl = document.getElementById('logs');

function renderLogs(logs) {
  logsEl.innerHTML = '';

  logs.forEach((log) => {
    const item = document.createElement('li');
    item.textContent = `#${log.index} | ${log.level} | delta ${Number(log.delta).toFixed(3)} | ${log.timestamp}`;
    logsEl.appendChild(item);
  });
}

ws.addEventListener('message', (message) => {
  const payload = JSON.parse(message.data);

  if (payload.type !== 'SYNC_STATE') {
    return;
  }

  countEl.textContent = `COUNT: ${payload.count}`;

  if (payload.latest) {
    levelEl.textContent = `LATEST LEVEL: ${payload.latest.level}`;
    deltaEl.textContent = `LATEST DELTA: ${Number(payload.latest.delta).toFixed(3)}`;
  } else {
    levelEl.textContent = 'LATEST LEVEL: -';
    deltaEl.textContent = 'LATEST DELTA: -';
  }

  renderLogs(payload.logs || []);
});
