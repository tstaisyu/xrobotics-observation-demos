# Log 005: Impact Event

## Purpose
Record when equipment receives impact during transport or installation, and show how strong that impact was. This log focuses on event history, damage signs, and making the state understandable for human operators.

## Structure
- `firmware/src/main.cpp`
  - Reads IMU acceleration `ax`, `ay`, `az`
  - Calculates `magnitude = sqrt(ax^2 + ay^2 + az^2)`
  - Calculates `delta = abs(magnitude - 1.0)`
  - Emits `IMPACT` events as JSON when `delta` exceeds the threshold
  - Classifies events into `LOW`, `MEDIUM`, `HIGH`
- `app/server.js`
  - Reads JSON lines from Serial
  - Broadcasts `IMPACT` events to browser clients over WebSocket
- `app/public/*`
  - Displays current impact level
  - Displays recent event history

## Event format
```json
{
  "type": "IMPACT",
  "level": "MEDIUM",
  "delta": 0.82,
  "timestamp": 123456
}
```

## Startup
```bash
cd demos/log-005-impact-event/app
npm install
SERIAL_PORT=/dev/ttyUSB0 npm start
```

Open `http://localhost:3000`.
