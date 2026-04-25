const express = require('express');
const http = require('http');
const { WebSocketServer } = require('ws');

const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const portPath = process.env.SERIAL_PORT || '/dev/ttyUSB0';

// --- Serial ---
const serial = new SerialPort({
  path: portPath,
  baudRate: 115200
});

const parser = serial.pipe(new ReadlineParser({ delimiter: '\n' }));

// --- Web ---
const app = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

app.use(express.static('public'));

let lastEvent = null;

parser.on('data', (line) => {
  try {
    const data = JSON.parse(line);
    lastEvent = data;

    console.log('EVENT:', data);

    // WebSocketで全クライアントに送信
    wss.clients.forEach((client) => {
      if (client.readyState === 1) {
        client.send(JSON.stringify(data));
      }
    });

  } catch (e) {
    console.log('RAW:', line);
  }
});

serial.on('open', () => {
  console.log('Serial connected:', portPath);
});

serial.on('error', (err) => {
  console.error('Serial error:', err.message);
});

// --- WebSocket接続時 ---
wss.on('connection', (ws) => {
  console.log('Client connected');

  if (lastEvent) {
    ws.send(JSON.stringify(lastEvent));
  }
});

server.listen(3000, () => {
  console.log('Server running: http://localhost:3000');
});