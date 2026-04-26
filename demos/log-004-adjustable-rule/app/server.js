const express = require('express');
const http = require('http');
const { WebSocketServer } = require('ws');

const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const portPath = process.env.SERIAL_PORT || '/dev/ttyUSB0';

const serial = new SerialPort({
  path: portPath,
  baudRate: 115200
});

const parser = serial.pipe(new ReadlineParser({ delimiter: '\n' }));

const app = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server });

app.use(express.static('public'));

let lastEvent = null;

function broadcast(data) {
  const payload = JSON.stringify(data);

  wss.clients.forEach((client) => {
    if (client.readyState === 1) {
      client.send(payload);
    }
  });
}

parser.on('data', (line) => {
  try {
    const data = JSON.parse(line);
    lastEvent = data;
    console.log('EVENT:', data);
    broadcast(data);
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

wss.on('connection', (ws) => {
  console.log('Client connected');

  if (lastEvent) {
    ws.send(JSON.stringify(lastEvent));
  }

  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message.toString());

      if (data.type === 'SET_LIMIT') {
        const limit = Number(data.value);

        if (Number.isInteger(limit) && limit >= 1 && limit <= 20) {
          serial.write(`LIMIT:${limit}\n`);
          console.log('SEND:', `LIMIT:${limit}`);
        }
      }

      if (data.type === 'RESET') {
        serial.write('RESET\n');
        console.log('SEND: RESET');
      }
    } catch (e) {
      console.error('WebSocket message error:', e.message);
    }
  });
});

server.listen(3000, () => {
  console.log('Server running: http://localhost:3000');
});