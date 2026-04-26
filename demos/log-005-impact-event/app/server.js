const express = require('express');
const http = require('http');
const path = require('path');
const { WebSocketServer } = require('ws');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const SERIAL_PORT = process.env.SERIAL_PORT || '/dev/ttyUSB0';
const BAUD_RATE = 115200;
const HTTP_PORT = 3000;

const app = express();
app.use(express.static(path.join(__dirname, 'public')));

const server = http.createServer(app);
const wss = new WebSocketServer({ server });

let lastImpact = null;

function broadcast(payload) {
  const serialized = JSON.stringify(payload);

  wss.clients.forEach((client) => {
    if (client.readyState === 1) {
      client.send(serialized);
    }
  });
}

const serial = new SerialPort({
  path: SERIAL_PORT,
  baudRate: BAUD_RATE,
});

const parser = serial.pipe(new ReadlineParser({ delimiter: '\n' }));

parser.on('data', (line) => {
  try {
    const event = JSON.parse(line.trim());

    if (event.type !== 'IMPACT') {
      return;
    }

    lastImpact = event;
    console.log('IMPACT:', event);
    broadcast(event);
  } catch (error) {
    console.error('Serial parse error:', error.message, '| line:', line.trim());
  }
});

serial.on('open', () => {
  console.log('Serial connected:', SERIAL_PORT);
});

serial.on('error', (error) => {
  console.error('Serial error:', error.message);
});

wss.on('connection', (ws) => {
  console.log('Client connected');

  if (lastImpact) {
    ws.send(JSON.stringify(lastImpact));
  }
});

server.listen(HTTP_PORT, () => {
  console.log(`Server running: http://localhost:${HTTP_PORT}`);
});
