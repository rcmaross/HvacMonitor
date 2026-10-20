#pragma once
// Minimal native-only Arduino shim. Never included in the ESP32 firmware build.
#include <stdint.h>
#include <math.h>
#include <string.h>
#define INPUT 0
unsigned long millis();
int analogRead(int pin);
void pinMode(int pin, int mode);
