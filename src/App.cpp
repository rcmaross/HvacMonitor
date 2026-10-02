#include "App.h"
#include <M5Unified.h>
//#include <math.h>

// ------------------------------------------------------------
// Sensor GPIOs
// ------------------------------------------------------------
const int LARGE_PIPE_PIN     = 25;
const int SMALL_PIPE_PIN     = 26;
const int OUTDOOR_AMBIENT_PIN = 13;

void App::addTemperatureSensor(const char* name, int pin)
{
    if (tempSensorCount >= MAX_TEMP_SENSORS)
    {
        Serial.printf(
            "FATAL: Cannot add temperature sensor '%s' - MAX_TEMP_SENSORS exceeded\n",
            name
        );

        abort();
    }
    tempSensors[tempSensorCount++] = new TemperatureSensor(name, pin);
}

TemperatureSensor *App::getTemperatureSensor(const char *name)
{
    for (size_t i = 0; i < tempSensorCount; i++)
    {
        if (strcmp(tempSensors[i]->name(), name) == 0)
        {
            return tempSensors[i];
        }
    }

    return nullptr;  
}

void App::begin()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200); 

    analogReadResolution(12);

    addTemperatureSensor("Large", LARGE_PIPE_PIN);
    addTemperatureSensor("Small", SMALL_PIPE_PIN);
    addTemperatureSensor("Outdoor", OUTDOOR_AMBIENT_PIN);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);

    M5.Display.setCursor(20, 20);
    M5.Display.println("AC Leak Detector");

    delay(1000);
}

void App::run()
{
        M5.update();

    TemperatureSensor* largeSensor = getTemperatureSensor("Large");
    float largePipe = largeSensor->readFahrenheit();
    TemperatureSensor* smallSensor = getTemperatureSensor("Small");
    float smallPipe = smallSensor->readFahrenheit();
    TemperatureSensor* outdoorSensor = getTemperatureSensor("Outdoor");
    float outdoor = outdoorSensor->readFahrenheit();
    // Serial output for debugging
    /*
    Serial.printf(
        "Large: %.2f F   Small: %.2f F   Outdoor: %.2f F\n",
        largePipe,
        smallPipe,
        outdoor
    );
   */
    // Update screen
    M5.Display.fillScreen(BLACK);

    M5.Display.setTextSize(2);

    M5.Display.setCursor(20, 20);
    M5.Display.println("AC Leak Detector");

    M5.Display.setTextSize(3);

    M5.Display.setCursor(20, 70);
    M5.Display.printf("Large:  %.1f F", largePipe);

    M5.Display.setCursor(20, 120);
    M5.Display.printf("Small:  %.1f F", smallPipe);

    M5.Display.setCursor(20, 170);
    M5.Display.printf("Outdoor: %.1f F", outdoor);

    delay(1000);
}