# Log 008: Impact Persistent Log

## Purpose
Keep impact event history even after power OFF/ON, so that transport or installation shocks can be reviewed later.

## Persistence choice
This log uses `NVS (Preferences)` for persistent storage.

## Structure
- `firmware/src/main.cpp`
  - Uses the same impact detection logic as Log 007
  - Stores `timestamp`, `delta`, and `level` in non-volatile storage
  - Loads saved logs at startup
  - Shows logs on-device using the same `LIVE / LOG` display modes
  - Clears saved data by holding `Button C` in `LIVE MODE`

## Button operations
- `Button A`
  - Previous log in `LOG MODE`
- `Button B`
  - Toggle `LIVE MODE` / `LOG MODE`
- `Button C`
  - Next log in `LOG MODE`
  - Hold in `LIVE MODE` to clear saved logs

## Verification steps
1. Build and write the firmware
2. Generate several impact events
3. Confirm `COUNT` increases in `LIVE MODE`
4. Power the device OFF
5. Power the device ON again
6. Confirm `COUNT` is still preserved at startup
7. Switch to `LOG MODE` and confirm old events are still visible
8. Hold `Button C` in `LIVE MODE` to clear saved logs

## Output format
```json
{"type":"IMPACT","level":"MEDIUM","delta":0.82,"timestamp":123456}
```
