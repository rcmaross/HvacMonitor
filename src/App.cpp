#include "App.h"
#include <M5Unified.h>
#include <esp_heap_caps.h>

extern "C" {
    extern char _data_start;
    extern char _data_end;
    extern char _bss_start;
    extern char _bss_end;
}
// ------------------------------------------------------------
// Sensor GPIOs
// ------------------------------------------------------------
const int LARGE_PIPE_PIN     = 25;
const int SMALL_PIPE_PIN     = 26;
const int OUTDOOR_AMBIENT_PIN = 13;

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
    
    setenv("TZ", tz_posix_rule.c_str(), 1);
    tzset(); // Force the system to update its local time offsets right now

    _lvgl.begin();
    _lvgl.printLvglMemory("begin");

    analogReadResolution(12);

    TemperatureSensor* large =
        addTemperatureSensor("Large", LARGE_PIPE_PIN);

    TemperatureSensor* small =
        addTemperatureSensor("Small", SMALL_PIPE_PIN);

    TemperatureSensor* outdoor =
        addTemperatureSensor("Outdoor", OUTDOOR_AMBIENT_PIN);

    _status = new Status(
        large,
        small,
        outdoor
    );

    _ui = new UIManager(*_status);
    printMemoryStats();
/*
    M5.Display.setRotation(1);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);

    M5.Display.setCursor(20, 20);
    M5.Display.println("AC Leak Detector");
    delay(1000);
*/
}

void App::runEverySecond()
{
    _status->update();
    _ui->update();

    _lvgl.printLvglMemory("loop");
}

void App::run()
{
    M5.update();

    _lvgl.update();

    static uint32_t lastUpdate = 0;
    uint32_t now = millis();

    if (now - lastUpdate >= ONESECOND_INTERVAL_MS)
    {
        lastUpdate = now;
        runEverySecond();
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
    Serial.printf(
        "  Globals/Statics: used=%u  limit=%u  free=%u  (%.1f%%)\n",
        staticSize,
        STATIC_DRAM_LIMIT,
        staticFree,
        staticPercent
    );

    Serial.printf(
        "                   data=%u  bss=%u\n",
        dataSize,
        bssSize
    );

    Serial.printf(
        "  Heap:            total=%u  free=%u  used=%u\n",
        heapTotal,
        heapFree,
        heapUsed
    );

    Serial.printf(
        "                   min_free=%u  largest=%u\n",
        heapMinFree,
        heapLargest
    );

    Serial.println();

    Serial.println("External PSRAM:");
    Serial.printf(
        "  Heap:            total=%u  free=%u  used=%u\n",
        psramTotal,
        psramFree,
        psramUsed
    );

    Serial.printf(
        "                   min_free=%u  largest=%u\n",
        psramMinFree,
        psramLargest
    );

    Serial.println("============================");
}