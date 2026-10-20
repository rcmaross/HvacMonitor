# Design decisions, limitations, and roadmap

This describes the **current implementation**, not promised future functionality.

## Compressor detection

Every 10 seconds `Status::sampleCompressor()` calculates slopes using three older temperature samples. In order:

1. Large pipe slope > 0.5 °C/min → `HEATING`.
2. Else large slope > 0.2 °C/min and large-minus-small >= 3 °C → `STANDBY_HEATING`.
3. Else absolute large slope <= 0.2 °C/min and large-minus-outdoor >= 20 °C → `PLATEAU`.
4. Else large slope < -0.2 °C/min and large-minus-small slope difference > 3 °C/min → `DOWNSTAGING`.
5. Else `OFF`.

Insufficient/invalid inputs → `UNKNOWN`. This is temperature-based inference, not proof of compressor electrical operation. `STANDBY_HEATING` currently counts as heating in analysis.

## Cycle classification

`AnalysisEngine` groups heating events, bridges OFF gaps <= `ignoreOff` (default 10 seconds), and classifies *cycle duration*: MAINT <= 180 seconds; LOW > 180 seconds and below HIGH threshold; HIGH >= threshold (default 40 × 30 seconds = 20 minutes). Automatic calibration seeks two duration clusters from recent non-MAINT cycles. These labels are **duration classes**, not verified inverter stages or efficiency levels.

## Known limitations

- Bridged OFF gaps currently contribute to ON runtime.
- `STANDBY_HEATING` is not separately represented as tentative evidence in `AnalysisEngine`.
- The compressor heuristic is under evaluation on conventional two-stage and inverter data.
- CSV files do not record whether their temperatures are Celsius or Fahrenheit.
- Invalid first ADC reading can seed the sensor's initial filter with NAN; needs testing.
- The touchscreen's 100% efficiency value is hard-coded, not computed.
- Local HTTP endpoints lack authentication.
- Existing ArduinoOTA accepts pushed firmware, but **no** device-initiated GitHub Release check/install, integrity validation workflow, or verified rollback feature exists.
- Current `App.cpp` pin mapping is for simulator loopback; verify real sensor wiring.

## Next work (not implemented)

Expand native/CSV regression tests; refine false-heating rejection without losing inverter MAINT cycles; create tagged GitHub release workflow and safe device-side OTA install UI; clarify units in versioned CSV metadata.

## Invariants

One firmware binary across compatible monitors; per-device preferences; one `App` owner; Celsius internal math; prompt SD writes; testable pure analysis engine; no automatic firmware installation.
