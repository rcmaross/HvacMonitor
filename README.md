# HvacMonitor

ESP32/M5Tough HVAC observation and analysis firmware. Three 10K B3950 NTC thermistors measure the large refrigerant pipe, small refrigerant pipe, and outdoor ambient temperature. The device estimates heating state, records SD-card CSV history, and presents status and recent cycle analysis on a local LVGL touchscreen and HTTP web UI.

**Experimental project:** Not a thermostat, safety controller, certified diagnostic instrument, or validated efficiency/COP meter. The touchscreen's current "100%" efficiency label is a placeholder, not a measurement.

## Documentation

- [Getting started](docs/GETTING_STARTED.md) — hardware, building, running, and deployment
- [Architecture](docs/ARCHITECTURE.md) — components, data flow, scheduling, persistence
- [Development](docs/DEVELOPMENT.md) — tests, simulator, build workflow, contributions
- [Design notes](docs/DESIGN_NOTES.md) — algorithm details, limitations, planned work
- [AI and contributor guide](AGENTS.md) — rapid repo orientation and change checklist

## Current features

- Three thermistor inputs, 60-sample trimmed-mean filtering.
- Compressor-state estimation: UNKNOWN, OFF, HEATING, STANDBY_HEATING, PLATEAU, DOWNSTAGING.
- Ten-second SD CSV logging and history replay for rolling 24-hour analysis.
- Heating-cycle duration classification: MAINT (up to 3 minutes), LOW, HIGH; optional automatic HIGH threshold calibration.
- Local touchscreen and web pages for status, analysis, history, and firmware metadata.
- ArduinoOTA **push** updates. GitHub Release discovery and on-device release installation are **not implemented**.
- M5Tough physical-loopback simulator for replay testing.

## Build

Install [PlatformIO](https://platformio.org/):

```sh
pio run -e m5stack-core2
pio test -e native
```

Building does not flash hardware. The USB upload port in `platformio.ini` is specifically configured for the maintainer's **simulator**. Production devices normally receive OTA pushes; do not assume the serial port targets production.

**Wiring warning:** The current `src/App.cpp` pin configuration uses loopback ADC GPIO35/36/34, not the older GPIO25/26/13 sensor mapping. Check [Getting started](docs/GETTING_STARTED.md) and actual hardware before flashing.

One firmware image is intended to serve both production monitors and the simulator. Configuration and deployment are device-specific; the firmware build is shared.

## Security and contributions

The local HTTP server has no authentication for history deletion and analysis configuration. Use on a trusted network only; do not expose it directly to the internet.

Contributions should include tests for behavioral changes and pass both native tests and ESP32 compilation. See [Development](docs/DEVELOPMENT.md).
