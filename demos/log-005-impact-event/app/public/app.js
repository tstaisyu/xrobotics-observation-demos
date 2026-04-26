const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
const ws = new WebSocket(`${protocol}://${window.location.host}`);

const levelEl = document.getElementById('level');
const deltaEl = document.getElementById('delta');
const timeEl = document.getElementById('time');
const logsEl = document.getElementById('logs');

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
  item.textContent = `${event.level} | delta ${event.delta.toFixed(3)} | ${new Date().toLocaleTimeString()}`;
  logsEl.prepend(item);

  if (logsEl.children.length > 10) {
    logsEl.removeChild(logsEl.lastChild);
  }
}

ws.addEventListener('message', (message) => {
  const event = JSON.parse(message.data);
  const level = event.level || 'NO EVENT';
  const delta = typeof event.delta === 'number' ? event.delta : 0;

  levelEl.textContent = level;
  levelEl.className = `level ${levelClass(level)}`;
  deltaEl.textContent = `delta: ${delta.toFixed(3)}`;
  timeEl.textContent = `time: ${new Date().toLocaleTimeString()}`;

  addLog(event);
});
