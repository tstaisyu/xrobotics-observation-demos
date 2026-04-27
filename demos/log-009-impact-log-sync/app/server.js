const fs = require('fs');
const path = require('path');
const express = require('express');
const http = require('http');
const { WebSocketServer } = require('ws');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const SERIAL_PORT = process.env.SERIAL_PORT || '/dev/ttyUSB0';
const BAUD_RATE = 115200;
const HTTP_PORT = 3000;
const LOG_FILE = path.join(__dirname, 'synced_logs.json');
const RECONNECT_DELAY_MS = 2000;
const SYNC_TIMEOUT_MS = 10000;

const app = express();
app.use(express.static(path.join(__dirname, 'public')));

const server = http.createServer(app);
const wss = new WebSocketServer({ server });

let syncedLogs = [];
let isSyncing = false;
let incomingSyncLogs = [];
let serial = null;
let parser = null;
let reconnectTimer = null;
let syncInProgress = false;
let syncTimeout = null;
let nextSyncId = 1;

function latestEvent() {
  if (syncedLogs.length === 0) {
    return null;
  }

  return syncedLogs[syncedLogs.length - 1];
}

function saveLogsToFile() {
  fs.writeFileSync(LOG_FILE, JSON.stringify(syncedLogs, null, 2));
}

function loadLogsFromFile() {
  if (!fs.existsSync(LOG_FILE)) {
    return;
  }

  try {
    syncedLogs = JSON.parse(fs.readFileSync(LOG_FILE, 'utf8'));
    nextSyncId = syncedLogs.reduce((maxId, log) => {
      return Math.max(maxId, Number(log.syncId) || 0);
    }, 0) + 1;
  } catch (error) {
    console.error('Failed to load synced logs:', error.message);
    syncedLogs = [];
    nextSyncId = 1;
  }
}

function broadcastState() {
  const payload = JSON.stringify({
    type: 'SYNC_STATE',
    count: syncedLogs.length,
    latest: latestEvent(),
    logs: syncedLogs,
  });

  wss.clients.forEach((client) => {
    if (client.readyState === 1) {
      client.send(payload);
    }
  });
}

function broadcastMessage(type, message) {
  const payload = JSON.stringify({ type, message });

  wss.clients.forEach((client) => {
    if (client.readyState === 1) {
      client.send(payload);
    }
  });
}

function clearSyncTimeout() {
  if (syncTimeout) {
    clearTimeout(syncTimeout);
    syncTimeout = null;
  }
}

loadLogsFromFile();

function handleSerialLine(line) {
  const trimmed = line.trim();

  if (!trimmed) {
    return;
  }

  if (trimmed === 'SYNC_START') {
    isSyncing = true;
    incomingSyncLogs = [];
    return;
  }

  if (trimmed === 'SYNC_END') {
    isSyncing = false;
    syncInProgress = false;
    clearSyncTimeout();
    const syncId = nextSyncId++;
    const syncedAt = new Date().toISOString();
    const baseIndex = syncedLogs.length;
    const appendedLogs = incomingSyncLogs.map((log, index) => ({
      index: baseIndex + index,
      syncId,
      syncedAt,
      deviceIndex: log.index,
      type: log.type,
      level: log.level,
      delta: log.delta,
      timestamp: log.timestamp,
    }));
    syncedLogs = syncedLogs.concat(appendedLogs);
    saveLogsToFile();
    broadcastState();
    broadcastMessage('SYNC_COMPLETE', 'Synchronization completed');
    return;
  }

  if (trimmed === 'CLEAR_DONE') {
    broadcastMessage('CLEAR_DONE', 'Device logs cleared after sync');
    return;
  }

  try {
    const data = JSON.parse(trimmed);

    if (isSyncing) {
      incomingSyncLogs.push(data);
      return;
    }

    if (data.type === 'IMPACT') {
      return;
    }
  } catch (error) {
    console.error('Serial parse error:', error.message, '| line:', trimmed);
  }
}

function scheduleReconnect() {
  if (reconnectTimer) {
    return;
  }

  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    connectSerial();
  }, RECONNECT_DELAY_MS);
}

function sendSyncRequest() {
  if (!serial || !serial.isOpen) {
    broadcastMessage('SYNC_ERROR', 'Serial port is not connected');
    return;
  }

  if (syncInProgress) {
    broadcastMessage('SYNC_ERROR', 'Sync is already in progress');
    return;
  }

  incomingSyncLogs = [];
  isSyncing = false;
  syncInProgress = true;
  clearSyncTimeout();
  console.log('SEND: SYNC_REQUEST');
  serial.write('SYNC_REQUEST\n', (error) => {
    if (error) {
      syncInProgress = false;
      console.error('SYNC_REQUEST error:', error.message);
      broadcastMessage('SYNC_ERROR', 'Failed to send sync request');
      return;
    }

    syncTimeout = setTimeout(() => {
      syncInProgress = false;
      isSyncing = false;
      incomingSyncLogs = [];
      broadcastMessage('SYNC_ERROR', 'Sync timed out');
    }, SYNC_TIMEOUT_MS);
  });
}

function sendClearLogs() {
  if (!serial || !serial.isOpen) {
    broadcastMessage('SYNC_ERROR', 'Serial port is not connected');
    return;
  }

  console.log('SEND: CLEAR_LOGS');
  serial.write('CLEAR_LOGS\n', (error) => {
    if (error) {
      console.error('CLEAR_LOGS error:', error.message);
      broadcastMessage('SYNC_ERROR', 'Failed to send clear request');
    }
  });
}

function connectSerial() {
  serial = new SerialPort({
    path: SERIAL_PORT,
    baudRate: BAUD_RATE,
    autoOpen: false,
  });

  parser = serial.pipe(new ReadlineParser({ delimiter: '\n' }));
  parser.on('data', handleSerialLine);

  serial.on('open', () => {
    console.log('Serial connected:', SERIAL_PORT);
    broadcastMessage('SERIAL_STATUS', 'Serial connected');
  });

  serial.on('close', () => {
    console.error('Serial closed');
    syncInProgress = false;
    isSyncing = false;
    clearSyncTimeout();
    broadcastMessage('SERIAL_STATUS', 'Serial disconnected');
    scheduleReconnect();
  });

  serial.on('error', (error) => {
    console.error('Serial error:', error.message);
    syncInProgress = false;
    isSyncing = false;
    clearSyncTimeout();
    broadcastMessage('SERIAL_STATUS', `Serial error: ${error.message}`);
    scheduleReconnect();
  });

  serial.open((error) => {
    if (error) {
      console.error('Serial open error:', error.message);
      scheduleReconnect();
    }
  });
}

connectSerial();

wss.on('connection', (ws) => {
  ws.send(JSON.stringify({
    type: 'SYNC_STATE',
    count: syncedLogs.length,
    latest: latestEvent(),
    logs: syncedLogs,
  }));

  ws.on('message', (message) => {
    try {
      const payload = JSON.parse(message.toString());

      if (payload.type === 'SYNC_REQUEST') {
        sendSyncRequest();
        return;
      }

      if (payload.type === 'CLEAR_LOGS') {
        sendClearLogs();
      }
    } catch (error) {
      console.error('WebSocket message error:', error.message);
    }
  });
});

server.listen(HTTP_PORT, () => {
  console.log(`Server running: http://localhost:${HTTP_PORT}`);
});
