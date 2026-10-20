# Getting started

## Hardware

Target is M5Tough/ESP32 with three 10K B3950 NTC thermistors and optional SD card. Each thermistor is wired as:

```text
3.3V -- 10K fixed resistor -- ADC -- 10K NTC -- GND
```

**Verify pins before flashing.** Current `src/App.cpp` uses ADC GPIO35 (large pipe), GPIO36 (small pipe), GPIO34 (outdoor). The old physical-probe GPIO25/26/13 mapping is commented out. These active pins support the simulator's loopback setup: DAC GPIO25 → ADC35, DAC GPIO26 → ADC36, filtered PWM GPIO19 → ADC34. The shared firmware initializes `SensorSimulator`, but outputs remain inactive until a valid serial command. Do not connect simulated outputs to a live thermistor divider without reviewing wiring.

## Install and build

Install Git and PlatformIO Core or VS Code PlatformIO. Dependencies are pinned in `platformio.ini` (espressif32 7.1.3, M5Unified 0.2.25, LVGL 9.6.0).

```sh
git clone https://github.com/rcmaross/HvacMonitor.git
cd HvacMonitor
pio run -e m5stack-core2
pio test -e native
```

Build does **not** upload. `pio run -e m5stack-core2 -t upload` uses a maintainer-specific USB serial port for the simulator. Production monitors normally use ArduinoOTA push updates on the trusted LAN. GitHub Release check/install UI is **not implemented**. Keep one common firmware binary across compatible monitors.

Avoid active builds in iCloud Drive: synchronized `.pio` output can generate duplicate files.

## Running

Boot initializes hardware, settings, display, three sensors, analysis, SD history, network, ArduinoOTA, web server, RTC/NTP clock, and UI. Configure Wi-Fi through the touchscreen Network screen; Settings offers timezone, NTP and display units. Once connected, browse device IP to access `/`, `/status`, `/analysis`, `/history`, and `/firmware`.

History writes every 10 seconds to `/HvacMonitor/history_v5.csv`. History is immediately flushed and closed. If the SD card is unavailable, logging is unavailable. After reboot, analysis replays recent eligible CSV files in batches before resuming live analysis.

**CSV unit caveat:** Temperatures are logged using the device's current metric/imperial setting; files do not record units. Changing units without rotating/converting the file can misinterpret old rows.

## Troubleshooting

- No sensor reading: check divider orientation, ground, pins; ADC 0/4095 is invalid.
- UNKNOWN compressor state immediately after boot: expected until enough 10-second samples exist.
- Missing history: inspect SD startup logs.
- Analysis loading: wait for history replay to finish.
- Wrong 24-hour results: verify NTP, timezone, SD history units and replay.
- Inaccurate compressor classifications: record CSV and test the heuristic; it does not measure compressor current directly.
