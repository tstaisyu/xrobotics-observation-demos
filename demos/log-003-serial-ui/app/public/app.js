const ws = new WebSocket('ws://localhost:3000');

const stateEl = document.getElementById('state');
const deltaEl = document.getElementById('delta');
const timeEl = document.getElementById('time');
const logsEl = document.getElementById('logs');

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);

  // メイン表示
  stateEl.textContent = "STATE: " + data.state;
  stateEl.className = data.state;

  deltaEl.textContent = "delta: " + data.delta.toFixed(3);
  timeEl.textContent = "time: " + new Date().toLocaleTimeString();

  // ログ追加
  const li = document.createElement('li');
  li.textContent = JSON.stringify(data);

  logsEl.prepend(li);

  // 10件まで
  if (logsEl.children.length > 10) {
    logsEl.removeChild(logsEl.lastChild);
  }
};