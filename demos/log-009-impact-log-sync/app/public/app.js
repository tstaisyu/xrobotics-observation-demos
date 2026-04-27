const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
const ws = new WebSocket(`${protocol}://${window.location.host}`);

const countEl = document.getElementById('count');
const levelEl = document.getElementById('level');
const deltaEl = document.getElementById('delta');
const logsEl = document.getElementById('logs');
const statusEl = document.getElementById('status');
const syncButton = document.getElementById('syncButton');

function renderLogs(logs) {
  logsEl.innerHTML = '';
  let currentSyncId = null;

  logs.forEach((log) => {
    if (log.syncId !== currentSyncId) {
      currentSyncId = log.syncId;
      const header = document.createElement('li');
      header.className = 'sync-group';
      header.textContent = `SYNC ${log.syncId} | ${log.syncedAt}`;
      logsEl.appendChild(header);
    }

    const item = document.createElement('li');
    item.textContent = `#${log.index} | device ${log.deviceIndex} | ${log.level} | delta ${Number(log.delta).toFixed(3)} | ${log.timestamp}`;
    logsEl.appendChild(item);
  });
}

syncButton.addEventListener('click', () => {
  const ok = window.confirm('M5内のログをPCへ同期します。同期成功後、M5内のログを削除します。よろしいですか？');
  if (!ok) {
    return;
  }

  statusEl.textContent = 'STATUS: syncing...';
  ws.send(JSON.stringify({ type: 'SYNC_REQUEST' }));
});

ws.addEventListener('message', (message) => {
  const payload = JSON.parse(message.data);

  if (payload.type === 'SYNC_COMPLETE') {
    statusEl.textContent = 'STATUS: sync completed';
    statusEl.textContent = 'STATUS: sync completed, clearing device logs...';
    ws.send(JSON.stringify({ type: 'CLEAR_LOGS' }));
    return;
  }

  if (payload.type === 'CLEAR_DONE') {
    statusEl.textContent = `STATUS: ${payload.message}`;
    return;
  }

  if (payload.type === 'SERIAL_STATUS' || payload.type === 'SYNC_ERROR') {
    statusEl.textContent = `STATUS: ${payload.message}`;
    return;
  }

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
