# Log 007: Impact Log Viewer

## Purpose
Extend the local impact logging flow so that saved events can be reviewed directly on the M5Stack itself, without a PC or browser UI always connected.

## Structure
- `firmware/src/main.cpp`
  - Detects impact events from IMU acceleration
  - Classifies each event as `LOW`, `MEDIUM`, or `HIGH`
  - Stores events in RAM
  - Uses M5 buttons to switch between live view and log view

## Button operations
- `Button A`
  - Move to the previous log entry in `LOG MODE`
- `Button B`
  - Toggle display mode
  - `LIVE MODE`: show latest event and total count
  - `LOG MODE`: show one saved event at a time
- `Button C`
  - Move to the next log entry in `LOG MODE`

## Screen behavior
- `LIVE MODE`
  - `COUNT`
  - `LAST`
  - `DELTA`
- `LOG MODE`
  - `LOG n / total`
  - `LEVEL`
  - `DELTA`
  - `TIME`

## Verification steps
1. Build and write the firmware
2. Generate several impact events by moving or tapping the device
3. Confirm `LIVE MODE` shows the latest level and total count
4. Press `Button B` to switch to `LOG MODE`
5. Press `Button A` and `Button C` to move through saved events
6. Confirm `LOG MODE` shows `index`, `level`, `delta`, and `timestamp`
7. Confirm Serial outputs JSON only when an impact event occurs
