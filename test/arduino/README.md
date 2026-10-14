# Arduino Mega 2560 HVAC temperature simulator

A standalone PlatformIO project for replaying HVAC temperatures into the
**unmodified** M5Tough ADC inputs. The supplied DiagDeck daughter-card driver
is in `src/DiagDeck.cpp` and `src/DiagDeck.h`.

## Wiring — do not connect the 5V PWM pins directly to the M5Tough

Each output requires a 10k series resistor, 15k shunt resistor, and 10uF
capacitor. All grounds must be connected.

```text
Mega PWM 8  --- 10k ---+--- M5Tough GPIO35 (large)
                       |
                      15k
                       |
                      GND
                       |
                 (10uF capacitor also from node to GND)

Mega PWM 9  --- same circuit --- M5Tough GPIO36 (small)
Mega PWM 10 --- same circuit --- M5Tough GPIO34 (outdoor)
Mega GND ---------------------- M5Tough GND
```

Capacitor positive lead goes to the ADC node, negative to GND.
The divider limits the filtered output to approximately 3.0V even if
PWM is accidentally set to 100%. The PWM is filtered, but not a precision
DAC; check actual voltages and temperatures before trusting slope tests.
The simulator computes voltages for the HVAC monitor's 10k/B3950 NTC
and 10k fixed resistor, with a nominal 3.3V divider supply.

**Important:** The existing HVAC monitor uses ADC counts (0..4095) to
calculate resistance; real ESP32 ADC response and attenuation are not
perfectly linear. Calibrate measured temperatures if necessary.

## Build / upload

From `test/arduino`:

```bash
pio run
pio run -t upload
```

## Serial replay

```bash
python3 -m pip install pyserial
python3 replay.py history_v3.csv --port /dev/cu.usbmodemXXXX
```

CSV must contain columns `timestamp,large,small,outside`, **in Fahrenheit**.
The history file follows the HVAC monitor's configured temperature units;
if it was recorded in Celsius, convert it before replaying.

The script preserves timestamp gaps where parseable; otherwise it uses
10-second intervals. The first row is sent immediately. Avoid concurrent
PlatformIO serial monitors because the serial port can have only one owner.

Protocol: `F,<large Fahrenheit>,<small Fahrenheit>,<outside Fahrenheit>\n`.
The Mega accepts one row at a time, sets all three PWM outputs, and replies
`OK`. After 30 seconds without accepted data it zeros the outputs.

## DiagDeck controls

- Button A (pin 46): pause/resume incoming updates.
- Button B (pin 48): toggle LCD between pipe temperatures and outside/status.
- LED 1: red while waiting/stale, green while receiving data.
- LED 2: green running, blue paused.

For now the replay script sends rows regardless of pause state; paused rows
are discarded. Resuming picks up the next arriving row.
