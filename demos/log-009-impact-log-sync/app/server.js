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

const app = express();
app.use(express.static(path.join(__dirname, 'public')));

const server = http.createServer(app);
const wss = new WebSocketServer({ server });

let syncedLogs = [];
let isSyncing = false;

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
  } catch (error) {
    console.error('Failed to load synced logs:', error.message);
    syncedLogs = [];
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

loadLogsFromFile();

const serial = new SerialPort({
  path: SERIAL_PORT,
  baudRate: BAUD_RATE,
});

const parser = serial.pipe(new ReadlineParser({ delimiter: '\n' }));

parser.on('data', (line) => {
  const trimmed = line.trim();

  if (!trimmed) {
    return;
  }

  if (trimmed === 'SYNC_START') {
    isSyncing = true;
    syncedLogs = [];
    return;
  }

  if (trimmed === 'SYNC_END') {
    isSyncing = false;
    saveLogsToFile();
    broadcastState();
    return;
  }

  try {
    const data = JSON.parse(trimmed);

    if (isSyncing) {
      syncedLogs.push(data);
      return;
    }

    if (data.type === 'IMPACT') {
      syncedLogs.push({
        index: syncedLogs.length,
        type: data.type,
        level: data.level,
        delta: data.delta,
        timestamp: data.timestamp,
      });
      saveLogsToFile();
      broadcastState();
    }
  } catch (error) {
    console.error('Serial parse error:', error.message, '| line:', trimmed);
  }
});

serial.on('open', () => {
  console.log('Serial connected:', SERIAL_PORT);
  serial.write('SYNC_REQUEST\n');
});

serial.on('error', (error) => {
  console.error('Serial error:', error.message);
});

wss.on('connection', (ws) => {
  ws.send(JSON.stringify({
    type: 'SYNC_STATE',
    count: syncedLogs.length,
    latest: latestEvent(),
    logs: syncedLogs,
  }));
});

server.listen(HTTP_PORT, () => {
  console.log(`Server running: http://localhost:${HTTP_PORT}`);
});
