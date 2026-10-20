# Development, tests, and simulator

## Daily workflow

```sh
git switch main && git pull
git switch -c feature/my-change
pio test -e native
pio run -e m5stack-core2
```

Create a PR to `main`. `.github/workflows/unit-tests.yml` runs native Unity tests first, then ESP32 firmware compilation, embedded version verification, development firmware artifact upload, and a static memory report. This is **not** a GitHub Releases pipeline.

The `native` PlatformIO environment excludes firmware sources by default; `test/test_analysis_engine/test_main.cpp` compiles the real `AnalysisEngine.cpp`. Do not implement duplicate algorithms in tests.

## Testing priorities

1. `AnalysisEngine`: cycle boundaries, OFF-gap bridging, window clipping, runtime percentages, calibration.
2. `Status`: HEATING, STANDBY_HEATING, PLATEAU, DOWNSTAGING, OFF, UNKNOWN; isolate pure detection logic before host tests.
3. `TempSensor`: ADC conversion, beta equation, trimmed average, invalid startup ADC.
4. `Analysis` replay: malformed CSV, old/new states, duplicate timestamps, unit conversion, gaps.
5. `SensorSimulator`: parser, limits, output timeout; then network/settings/OTA integration tests.

Capture anonymized real CSV fixtures from conventional two-stage and inverter equipment. Run both sets after changing thresholds. Characterize existing behavior before fixing questionable behavior.

## Physical loopback simulator

The shared firmware accepts serial lines of the form:

```text
F,<largeF>,<smallF>,<outdoorF>
F,90,75,50
```

Valid finite values are -20°F to 200°F. `SensorSimulator` converts values to DAC GPIO25/26 and PWM GPIO19 (external RC filter), looped back to ADC GPIO35/36/34. It emits `READY`, `OK`, `SIM`, and `TEMP` diagnostics and zeros outputs after 30 seconds without a valid command. Inspect wiring before powering. Serial upload port in `platformio.ini` is the maintainer's simulator.

## Generated files

`scripts/generate_web.py` embeds `web/*.html` as generated PROGMEM headers. `scripts/build_info.py` embeds Git SHA, build date, and development/release version; development builds default to `vX.Y.Z-dev.<shortsha>`. Edit the sources, not generated headers.

## PR checklist

- Preserve one common firmware image for compatible monitors.
- Avoid blocking the main loop and excessive LVGL/internal RAM use.
- Preserve CSV replay compatibility or explicitly version the format.
- Keep hardware-independent calculations native-testable.
- Never silently change pin mappings or production OTA deployment.
- Do not claim CI/build/test success without checking the actual results.
- Update docs and tests for changes in behavior, API, or hardware.
