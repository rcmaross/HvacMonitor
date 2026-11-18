# ADS1115 hardware bring-up (M5Tough)

Standalone PlatformIO firmware to validate I2C communication and all four ADS1115 single-ended channels. This does not change or depend on the production HVAC Monitor firmware. It does **not** yet measure AC current or use ALERT/RDY.

## Wiring

Assumes the M5Tough external Port A I2C pins:

| M5Tough | ADS1115 |
| --- | --- |
| GPIO32 (SDA) | SDA |
| GPIO33 (SCL) | SCL |
| 3.3 V | VDD |
| GND | GND |
| GND | ADDR (I2C address 0x48) |

The ADS1115 breakout may already include SDA/SCL pull-ups. If yours does not, add suitable pull-ups to **3.3 V**. The existing ALERT/RDY pull-up can remain connected, but the test does not exercise ALERT/RDY.

**Important:** ADS1115 analog inputs must remain between GND and VDD. The configured +/-4.096 V PGA range does *not* permit a 4.096 V input when powered at 3.3 V. Never apply 5 V to the M5Tough I2C pins.

## Build and upload

From this directory:

```sh
pio run
pio run -t upload
pio device monitor -b 115200
```

If auto-detection fails, specify your serial port with `pio run -t upload --upload-port /dev/cu.usbserial-XXXX` and `pio device monitor -p /dev/cu.usbserial-XXXX -b 115200`.

## Expected results

The display reports detection at address 0x48 and live raw counts and voltages for A0 through A3; serial prints readings once per second. To validate an input, connect it to GND and then a known voltage within 0..3.3 V, and compare with the reported voltage. Floating channels may report arbitrary values. Each read is a single-shot conversion at 128 samples/s; this is an initial ADC communication test, not yet the final continuous-conversion/current-sampling design.

If your ADS1115 ADDR pin is strapped differently, change `ADS_ADDRESS` in `src/main.cpp`. GPIO pin assignments are also constants at the top of that file.
