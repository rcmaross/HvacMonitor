# AI assistant and contributor orientation

Read [README.md](README.md), [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md), and [docs/DESIGN_NOTES.md](docs/DESIGN_NOTES.md). **Inspect current branch code before editing.** This file is a navigation aid, not authoritative over source.

## Repository map

`src/main.cpp` creates global `App`; `App.cpp` owns initialization, pin mapping and scheduling. `TempSensor.*` reads and filters NTCs; `Status.*` detects compressor state; `Analysis.*` aggregates and replays; `AnalysisEngine.*` contains pure C++ cycle calculations; `History.*` handles SD CSV. `Network.*`, `Settings.*`, `Clock.*` handle device services. `UIManager.*`, `*UI.*`, `LVGLManager.*` drive touchscreen; `WebManager.*`, `StatusWeb.*`, `web/*.html` drive HTTP. `OTA.*` is ArduinoOTA push; `SensorSimulator.*` drives hardware loopback. `scripts/*.py` generates build metadata/web headers. `test/test_analysis_engine/` contains Unity tests.

## Run before proposing a merge

```sh
pio test -e native
pio run -e m5stack-core2
```

Report outcomes accurately; never assume unrun CI passed. Do not upload to deployed devices without authorization. The serial port in `platformio.ini` targets the maintainer's simulator. The same firmware image is intended for all compatible monitors.

## Important distinctions

- `CompressorState` (OFF/HEATING/STANDBY_HEATING/PLATEAU/DOWNSTAGING/UNKNOWN) is **not** `AnalysisEventType` (binary OFF/HEATING) and is **not** duration class (MAINT/LOW/HIGH).
- `STANDBY_HEATING` currently maps to a HEATING analysis event.
- Short OFF gaps bridged by `AnalysisEngine` currently count in ON totals.
- CSV units follow current settings, without historical unit metadata.
- The 100% efficiency label is a placeholder.
- GitHub Release discovery/install is planned, **not implemented**; ArduinoOTA push exists.
- `src/App.cpp` currently maps sensors to ADC35/36/34 for simulator loopback.
- Web API lacks authentication: trusted LAN only.

## Change protocol

Trace `TempSensor → Status → Analysis → AnalysisEngine → History/UI` before changing detection. Preserve real two-stage and inverter behavior, add native regression tests, respect LVGL/RAM constraints and cooperative scheduling, preserve CSV compatibility, and update docs. Prefer asking or inspecting over guessing.
