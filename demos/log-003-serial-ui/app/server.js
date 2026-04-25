const http = require('http');
const path = require('path');
const express = require('express');
const { WebSocketServer } = require('ws');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const SERIAL_PORT = process.env.SERIAL_PORT || '/dev/ttyUSB0';
const BAUD_RATE = 115200;
const HOST = '127.0.0.1';
const PORT = 3000;

const app = express();
app.use(express.static(path.join(__dirname, 'public')));

const server = http.createServer(app);
const wss = new WebSocketServer({ server });

function broadcast(payload) {
  const message = JSON.stringify(payload);
  wss.clients.forEach((client) => {
    if (client.readyState === 1) {
      client.send(message);
    }
  });
}

wss.on('connection', () => {
  console.log('[ws] client connected');
});

const serial = new SerialPort({
  path: SERIAL_PORT,
  baudRate: BAUD_RATE,
});

const parser = serial.pipe(new ReadlineParser({ delimiter: '\n' }));

serial.on('open', () => {
  console.log(`[serial] opened ${SERIAL_PORT} @ ${BAUD_RATE}`);
});

serial.on('error', (err) => {
  console.error('[serial] port error:', err.message);
});

parser.on('data', (line) => {
  const trimmed = line.trim();
  if (!trimmed) {
    return;
  }

  try {
    const parsed = JSON.parse(trimmed);
    if (!parsed || typeof parsed !== 'object') {
      return;
    }

    broadcast(parsed);
  } catch (err) {
    console.error('[serial] JSON parse error:', err.message, '| line:', trimmed);
  }
});

server.listen(PORT, HOST, () => {
  console.log(`[http] http://${HOST}:${PORT}`);
  console.log(`[serial] waiting on ${SERIAL_PORT}`);
});
