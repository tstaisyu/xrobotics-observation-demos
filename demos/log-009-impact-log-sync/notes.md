# Log 009: Impact Log Sync

## Purpose
Synchronize impact logs stored persistently on the M5Stack to a connected PC over USB Serial, then save and display them on the Node.js side.

## Structure
- `firmware/src/main.cpp`
  - Uses the same impact detection and persistent storage logic as Log 008
  - Responds to `SYNC_REQUEST` by outputting saved logs between `SYNC_START` and `SYNC_END`
- `app/server.js`
  - Opens Serial
  - Sends `SYNC_REQUEST` automatically
  - Receives synced logs and saves them to `synced_logs.json`
  - Broadcasts logs to browser clients over WebSocket
- `app/public/*`
  - Displays synced count, latest level, latest delta, and full log list

## Sync output format
```text
SYNC_START
{"index":0,"type":"IMPACT","level":"LOW","delta":0.42,"timestamp":123456}
{"index":1,"type":"IMPACT","level":"HIGH","delta":1.31,"timestamp":124980}
SYNC_END
```

## Verification steps
1. Write the firmware to M5
2. Generate impact events while the device is disconnected from PC UI
3. Connect the M5 to PC
4. Run:
```bash
cd demos/log-009-impact-log-sync/app
npm install
SERIAL_PORT=/dev/ttyUSB0 npm start
```
5. Confirm the server sends `SYNC_REQUEST`
6. Confirm the M5 returns `SYNC_START` ... `SYNC_END`
7. Open `http://localhost:3000`
8. Confirm `COUNT`, latest level, latest delta, and synced logs are displayed
