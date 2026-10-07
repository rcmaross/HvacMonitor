#include "OTA.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

void OTA::begin()
{
    uint64_t mac = ESP.getEfuseMac();

    char hostname[32];

    snprintf(
        hostname,
        sizeof(hostname),
        "hvac-monitor-%06X",
        (uint32_t)(mac & 0xFFFFFF)
    );

    ArduinoOTA.setHostname(hostname);

    Serial.printf("OTA hostname: %s.local\n", hostname);

    ArduinoOTA.onStart([]()
    {
        Serial.println("OTA update starting");
    });

    ArduinoOTA.onEnd([]()
    {
        Serial.println("\nOTA update complete");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        Serial.printf(
            "OTA progress: %u%%\r",
            (progress * 100U) / total
        );
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("\nOTA error %u\n", error);
    });

    ArduinoOTA.begin();

    Serial.println("OTA ready");
}

void OTA::update()
{
    ArduinoOTA.handle();
}