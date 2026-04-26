# Log 006: Impact Local Log

## Purpose
Record impact events on the M5Stack itself, even when no PC or browser UI is continuously connected. This log focuses on storing impact history in memory and exporting it later over Serial.

## Structure
- `firmware/src/main.cpp`
  - Reads IMU acceleration `ax`, `ay`, `az`
  - Calculates `magnitude = sqrt(ax^2 + ay^2 + az^2)`
  - Calculates `delta = abs(magnitude - 1.0)`
  - Detects `IMPACT` when `delta` exceeds the threshold
  - Classifies impact as `LOW`, `MEDIUM`, or `HIGH`
  - Stores `timestamp`, `delta`, and `level` in RAM
  - Exports saved logs over Serial using commands

## Serial commands
- `LIST`
  - Outputs all saved impact events as JSON Lines
- `CLEAR`
  - Clears all saved impact events
- `COUNT`
  - Outputs the current saved event count

## Output examples
```json
{"type":"IMPACT","level":"MEDIUM","delta":0.82,"timestamp":123456}
{"index":0,"type":"IMPACT","level":"LOW","delta":0.42,"timestamp":123456}
{"index":1,"type":"IMPACT","level":"HIGH","delta":1.31,"timestamp":124980}
{"type":"COUNT","count":2}
```

## Verification steps
1. Build and write the firmware
2. Move or tap the device to generate impact events
3. Confirm that the LCD shows the latest level and saved count
4. Open the serial monitor at `115200`
5. Send `COUNT` and verify the saved event count
6. Send `LIST` and verify all saved events are output
7. Send `CLEAR` and verify the log is cleared
