const stateText = document.getElementById('stateText');
const deltaText = document.getElementById('deltaText');
const timeText = document.getElementById('timeText');
const logList = document.getElementById('logList');
const statusCard = document.getElementById('statusCard');

const events = [];

function formatTime(date) {
  return date.toLocaleTimeString();
}

function renderLogs() {
  logList.innerHTML = '';

  events.slice(0, 10).forEach((event) => {
    const item = document.createElement('li');
    item.className = event.state === 'MOTION_SPIKE' ? 'spike' : 'stable';
    item.innerHTML = `<span>${event.state} | delta=${event.delta.toFixed(3)}</span><span>${event.receivedAt}</span>`;
    logList.appendChild(item);
  });
}

function setStateVisual(state) {
  statusCard.classList.remove('status-stable', 'status-spike');
  if (state === 'MOTION_SPIKE') {
    statusCard.classList.add('status-spike');
  } else if (state === 'STABLE') {
    statusCard.classList.add('status-stable');
  }
}

function handleEvent(payload) {
  const state = payload.state || 'UNKNOWN';
  const delta = Number(payload.delta);

  stateText.textContent = state;
  deltaText.textContent = Number.isFinite(delta) ? delta.toFixed(3) : '-';

  const now = new Date();
  const nowText = formatTime(now);
  timeText.textContent = nowText;
  setStateVisual(state);

  events.unshift({
    state,
    delta: Number.isFinite(delta) ? delta : 0,
    receivedAt: nowText,
  });

  if (events.length > 10) {
    events.length = 10;
  }

  renderLogs();
}

const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
const socket = new WebSocket(`${protocol}://${window.location.host}`);

socket.addEventListener('open', () => {
  console.log('[ws] connected');
});

socket.addEventListener('message', (event) => {
  try {
    const payload = JSON.parse(event.data);
    handleEvent(payload);
  } catch (err) {
    console.error('[ws] invalid message:', err.message);
  }
});

socket.addEventListener('error', (event) => {
  console.error('[ws] error:', event);
});

socket.addEventListener('close', () => {
  console.error('[ws] disconnected');
});
