#include "App.h"
#include <M5Unified.h>
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <sys/time.h>
#include <time.h>

extern "C" {
    extern char _data_start;
    extern char _data_end;
    extern char _bss_start;
    extern char _bss_end;
}
// ------------------------------------------------------------
// Sensor GPIOs
// ------------------------------------------------------------
//const int LARGE_PIPE_PIN     = 25;
//const int SMALL_PIPE_PIN     = 26;
//const int OUTDOOR_AMBIENT_PIN = 13;
const int LARGE_PIPE_PIN     = 35;
const int SMALL_PIPE_PIN     = 36;
const int OUTDOOR_AMBIENT_PIN = 34;

TemperatureSensor* App::addTemperatureSensor(
    const char* name,
    int pin
)
{
    if (tempSensorCount >= MAX_TEMP_SENSORS)
    {
        Serial.printf(
            "FATAL: Cannot add temperature sensor '%s' - MAX_TEMP_SENSORS exceeded\n",
            name
        );

        abort();
    }

    TemperatureSensor* sensor =
        new TemperatureSensor(name, pin);

    tempSensors[tempSensorCount++] = sensor;

    return sensor;
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
    Serial.printf("App starting\n");

    M5.Power.setExtOutput(true);
    M5.Power.setChargeCurrent(100); // charge RTC battery
    
    _settings = new Settings();
    _settings->begin();

    _lvgl.begin();
    _lvgl.printLvglMemory("begin");

    analogReadResolution(12);
    TemperatureSensor* large = addTemperatureSensor("Large", LARGE_PIPE_PIN);

    TemperatureSensor* small = addTemperatureSensor("Small", SMALL_PIPE_PIN);

    TemperatureSensor* outdoor = addTemperatureSensor("Outdoor", OUTDOOR_AMBIENT_PIN);

    _status = new Status(large, small, outdoor);
    _status->begin();

    _network = new Network(); 
    _network->begin();

    _ota = new OTA();
    _ota->begin();


    _web = new WebManager(*_status);
    _web->begin();

    _clock = new Clock(*_settings, *_network);

    _clock->begin();
    _ui = new UIManager(*_status, *_network, *_settings, *_clock);
    printMemoryStats();
}

void App::runEveryQuarterSecond()
{
    _status->update();
}

float App::convertTempFromCelsius(float c)
{
    if (_settings->useMetric())
        return c;

    return c * 9.0f / 5.0f + 32.0f;
}

void App::runEverySecond()
{
    _clock->update();
    _ui->update();

    //_lvgl.printLvglMemory("loop");
}

void App::run()
{
    M5.update();

    _lvgl.update();
    _network->update();
    _ota->update();
    _web->update();

    static uint32_t lastSecondUpdate = 0;
    static uint32_t lastQuarterSecondUpdate = 0;

    uint32_t now = millis();

    if (now - lastSecondUpdate >= ONESECOND_INTERVAL_MS)
    {
        lastSecondUpdate = now;
        runEverySecond();
    }

    if (now - lastQuarterSecondUpdate >= QUARTERSECOND_INTERVAL_MS)
    {
        lastQuarterSecondUpdate = now;
        runEveryQuarterSecond();
    }


    delay(5);
}

void App::printMemoryStats()
{
    static constexpr size_t STATIC_DRAM_LIMIT = 160 * 1024;

    Serial.println("========== MEMORY ==========");

    // --------------------------------------------------------
    // Static allocations in internal SRAM
    // --------------------------------------------------------

    size_t dataSize =
        reinterpret_cast<uintptr_t>(&_data_end) -
        reinterpret_cast<uintptr_t>(&_data_start);

    size_t bssSize =
        reinterpret_cast<uintptr_t>(&_bss_end) -
        reinterpret_cast<uintptr_t>(&_bss_start);

    size_t staticSize = dataSize + bssSize;
    size_t staticFree =
        staticSize < STATIC_DRAM_LIMIT
            ? STATIC_DRAM_LIMIT - staticSize
            : 0;

    float staticPercent =
        100.0f * staticSize / STATIC_DRAM_LIMIT;

    // --------------------------------------------------------
    // Normal Arduino heap
    // --------------------------------------------------------

    size_t heapTotal   = ESP.getHeapSize();
    size_t heapFree    = ESP.getFreeHeap();
    size_t heapUsed    = heapTotal - heapFree;
    size_t heapMinFree = ESP.getMinFreeHeap();
    size_t heapLargest = ESP.getMaxAllocHeap();

    // --------------------------------------------------------
    // PSRAM
    // --------------------------------------------------------

    size_t psramTotal   = ESP.getPsramSize();
    size_t psramFree    = ESP.getFreePsram();
    size_t psramUsed    = psramTotal - psramFree;
    size_t psramMinFree = ESP.getMinFreePsram();
    size_t psramLargest = ESP.getMaxAllocPsram();

    // --------------------------------------------------------
    // Display
    // --------------------------------------------------------

    Serial.println("Internal SRAM:");
    Serial.printf("  Globals/Statics: used=%u  limit=%u  free=%u  (%.1f%%)\n", staticSize, STATIC_DRAM_LIMIT, staticFree, staticPercent);

    Serial.printf("                   data=%u  bss=%u\n", dataSize, bssSize);

    Serial.printf("  Heap:            total=%u  free=%u  used=%u\n", heapTotal, heapFree, heapUsed);

    Serial.printf("                   min_free=%u  largest=%u\n", heapMinFree, heapLargest);

    Serial.println();

    Serial.println("External PSRAM:");
    Serial.printf("  Heap:            total=%u  free=%u  used=%u\n", psramTotal, psramFree, psramUsed);

    Serial.printf(
        "                   min_free=%u  largest=%u\n", psramMinFree, psramLargest);

    Serial.println("============================");
}
