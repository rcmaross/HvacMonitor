# Architecture and data flow

```text
NTC thermistors → TempSensor (ADC + trimmed mean)
               → Status (temperatures, slopes, compressor state)
                 ├→ History (SD CSV every 10s)
                 ├→ Analysis (events, replay) → AnalysisEngine (pure C++ cycles)
                 └→ StatusUI / StatusWeb
Analysis → AnalysisUI / WebManager
```

`src/main.cpp` declares a single global `App app`. `App::begin()` owns initialization and subsystem lifetime. Avoid introducing a second global system state.

## Main loop schedule

| Interval | Work |
|---|---|
| Every loop | M5, simulator, LVGL, Wi-Fi, ArduinoOTA, HTTP, analysis background work |
| ~250 ms | `Status::update()` sensor readings; simulator feedback |
| ~1 second | Clock/NTP and UI update |
| ~10 seconds | `Status::sampleCompressor()`, `Analysis::sample()`, `History::write()` |

This is cooperative `millis()` scheduling, not hard real time. Avoid blocking the loop.

## Source map

- `App.*`: ownership, pin mapping, scheduling.
- `TempSensor.*`: thermistor beta equation, 60-sample trimmed mean (discard 3 highest and 3 lowest).
- `Status.*`: 3-sample temperature slopes and instantaneous compressor states.
- `Analysis.*`: sample aggregation, PSRAM event ring (12,000 events), 24-hour summary, settings, CSV replay.
- `AnalysisEngine.*`: hardware-independent cycle reconstruction, MAINT/LOW/HIGH classification, auto-calibration.
- `History.*`: mount SD, write/flush CSV, file management, batch replay.
- `Network.*`, `Settings.*`, `Clock.*`: Wi-Fi, Preferences, RTC/NTP.
- `UIManager.*`, `*UI.*`, `LVGLManager.*`: touchscreen.
- `WebManager.*`, `StatusWeb.*`, `web/*.html`: HTTP and web pages.
- `OTA.*`: ArduinoOTA **push receiver**, not a release downloader.
- `SensorSimulator.*`: serial-driven physical loopback.
- `scripts/build_info.py`, `scripts/generate_web.py`: build-time metadata and embedded web headers.

## Persistence and replay

Preferences namespaces: `settings`, `network`, `analysis`. CSV: `/HvacMonitor/history_v5.csv`. Internal temperatures/slopes are Celsius/Celsius per minute; CSV values use the selected display units. History writes and flushes each 10-second record; no deferred queue.

At startup, `Analysis` replays recent CSV in batches of up to 150 lines per loop, then switches to live input. Replay rejects duplicate/out-of-order timestamps. The CSV unit system is inferred from *current* settings, a known limitation.

`Analysis::feed()` maps `OFF` to OFF and every other known state (including `STANDBY_HEATING`) to HEATING. The pure engine bridges short OFF gaps for cycle classification; presently those gaps also count toward ON seconds. Both behaviors are deliberate test targets, not proven ideal.

## Web endpoints

`/api/status`, `/api/analysis`, `/api/firmware`, `/api/history/files` are GET endpoints. `/api/analysis/config` and `/api/history/delete` are POST endpoints; `/api/history/download?name=...` downloads a file. These endpoints do not implement authentication. Trusted LAN only.

To add a page, edit `web/*.html`; the build script generates `include/generated/*_html.h`. Do not hand-edit generated headers.
