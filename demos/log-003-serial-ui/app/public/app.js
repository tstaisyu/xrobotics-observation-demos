const ws = new WebSocket('ws://localhost:3000');

const stateEl = document.getElementById('state');
const deltaEl = document.getElementById('delta');
const timeEl = document.getElementById('time');
const logsEl = document.getElementById('logs');

function eventToJapanese(event) {
  switch (event) {
    case "MOTION_SPIKE": return "衝撃検知";
    case "ENTER_PROTECT_MODE": return "保護モード移行";
    case "PROTECT_MODE_ACTIVE": return "保護中の衝撃";
    case "RESET": return "リセット";
    default: return event;
  }
}

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);

  const mode = data.mode || '-';
  const eventName = eventToJapanese(data.event || '-');
  const spikeCount = data.spikeCount ?? 0;
  const delta = typeof data.delta === 'number' ? data.delta : 0;

  stateEl.textContent = `状態: ${mode} / ${eventName} / 回数: ${spikeCount}`;
  stateEl.className = mode === "保護モード" ? "PROTECT_MODE" : "MONITORING";

  deltaEl.textContent = `変化量: ${delta.toFixed(3)}`;
  timeEl.textContent = `時刻: ${new Date().toLocaleTimeString()}`;

  const li = document.createElement('li');
  li.textContent = `${eventToJapanese(data.event)} (${delta.toFixed(3)})`;
  logsEl.prepend(li);

  if (logsEl.children.length > 10) {
    logsEl.removeChild(logsEl.lastChild);
  }
};