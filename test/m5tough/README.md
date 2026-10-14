# M5Tough HVAC temperature simulator

Standalone PlatformIO project. Runs on a second M5Tough, receives temperatures
over USB serial from a Mac, and drives the lab HVAC Monitor's **existing**
thermistor ADC inputs. No Wi-Fi or changes to production monitor firmware.

## Connections

| Simulator M5Tough | Lab HVAC Monitor M5Tough | Output |
| --- | --- | --- |
| GPIO25 (DAC1) | GPIO35 (large pipe) | Analog DAC |
| GPIO26 (DAC2) | GPIO36 (small pipe) | Analog DAC |
| GPIO19 (PWM) | GPIO34 (outside) | **RC filter required** |
| GND | GND | Shared ground |

**GPIO19 cannot be connected directly** if you want a steady analog voltage.
Install a 10k resistor in series from GPIO19 to GPIO34 and a 10uF capacitor
from GPIO34 to GND (capacitor + to GPIO34, - to GND). This is one resistor
and one capacitor. GPIO25/26 are actual DACs and do not require filters.
All outputs are 3.3V-domain signals; never connect 5V.

The receiving M5Tough must have **no external thermistor divider attached**
to these three ADC inputs. Confirm pin accessibility and wiring before
powering up.

**Accuracy limitation:** ESP32 DAC1/DAC2 are 8-bit and not precision
voltage references. The PWM channel is also 8-bit in this implementation.
The production TempSensor uses raw ESP32 ADC counts in its resistance
equation, so real measured temperatures may differ from the commanded
temperatures. Verify with a meter and the lab monitor before trusting
small slope thresholds. We can calibrate or upgrade resolution later.

## PlatformIO

Open the `test/m5tough` directory as its own PlatformIO project, then:

```sh
pio run
pio run -t upload
```

## Replay from Mac

```sh
python3 -m pip install pyserial
python3 replay.py history_v3.csv --port /dev/cu.usbserial-XXXX --units F
```

The script reads `timestamp,large,small,outside` from the HVAC Monitor's
history CSV. Temperatures in the history CSV follow the monitor's display
units, so pass `--units C` if recorded in Celsius. Timestamp gaps are
replayed at their original durations, with 10 seconds as fallback. Use
`--speed 2` for 2x playback.

Serial protocol at 115200 baud: `F,75.0,95.0,65.0\n`. The simulator
acknowledges each row with `OK ...`; invalid rows get `ERR ...`.
If no valid command arrives for 30 seconds, outputs are set to zero.
Do not run a separate serial monitor simultaneously with replay.py.

For initial bring-up, use a single manual serial command and verify the
three voltages before connecting the lab monitor. A 0V timeout output is
not a valid simulated temperature; it is a fail-safe state.
