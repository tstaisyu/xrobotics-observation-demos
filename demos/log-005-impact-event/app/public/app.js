const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
const ws = new WebSocket(`${protocol}://${window.location.host}`);

const levelEl = document.getElementById('level');
const deltaEl = document.getElementById('delta');
const timeEl = document.getElementById('time');
const logsEl = document.getElementById('logs');

function levelLabel(level) {
  if (level === 'HIGH') {
    return '高';
  }

  if (level === 'MEDIUM') {
    return '中';
  }

  if (level === 'LOW') {
    return '低';
  }

  return '待機中';
}

function levelClass(level) {
  if (level === 'HIGH') {
    return 'level-high';
  }

  if (level === 'MEDIUM') {
    return 'level-medium';
  }

  if (level === 'LOW') {
    return 'level-low';
  }

  return 'level-none';
}

function addLog(event) {
  const item = document.createElement('li');
  item.textContent = `衝撃: ${levelLabel(event.level)} | 変化量 ${event.delta.toFixed(3)} | ${new Date().toLocaleTimeString()}`;
  logsEl.prepend(item);

  if (logsEl.children.length > 10) {
    logsEl.removeChild(logsEl.lastChild);
  }
}

function showWaiting() {
  levelEl.textContent = '待機中';
  levelEl.className = 'level level-none';
  deltaEl.textContent = '変化量: -';
  timeEl.textContent = '最終受信: -';
}

showWaiting();

ws.addEventListener('message', (message) => {
  const event = JSON.parse(message.data);
  const level = event.level || '';
  const delta = typeof event.delta === 'number' ? event.delta : 0;

  levelEl.textContent = levelLabel(level);
  levelEl.className = `level ${levelClass(level)}`;
  deltaEl.textContent = `変化量: ${delta.toFixed(3)}`;
  timeEl.textContent = `最終受信: ${new Date().toLocaleTimeString()}`;

  addLog(event);
});
