#include "StatusWeb.h"
#include "App.h"
#include "WiFi.h"
#include <math.h>

StatusWeb::StatusWeb(Status& status)
    : _status(status)
{
}

String StatusWeb::statusJson() const
{
    float large =
        app.convertTempFromCelsius(_status.largePipeTemperature());

    float small =
        app.convertTempFromCelsius(_status.smallPipeTemperature());

    float outdoor =
        app.convertTempFromCelsius(_status.outdoorTemperature());

    bool haveTemperature = false;
    float lowTemp = 0.0f;
    float highTemp = 0.0f;

    const float temperatures[] =
    {
        large,
        small,
        outdoor
    };

    for (float temperature : temperatures)
    {
        if (isnan(temperature))
            continue;

        if (!haveTemperature)
        {
            lowTemp = temperature;
            highTemp = temperature;
            haveTemperature = true;
        }
        else
        {
            if (temperature < lowTemp)
                lowTemp = temperature;

            if (temperature > highTemp)
                highTemp = temperature;
        }
    }

    float displayMin = 0.0f;
    float displayMax = 100.0f;

    if (haveTemperature)
    {
        constexpr float MIN_DISPLAY_SPAN = 20.0f;
        constexpr float SCALE_PADDING = 5.0f;
        constexpr float SCALE_INCREMENT = 5.0f;

        float requiredSpan =
            (highTemp - lowTemp) +
            (2.0f * SCALE_PADDING);

        float displaySpan =
            max(requiredSpan, MIN_DISPLAY_SPAN);

        float center =
            (lowTemp + highTemp) / 2.0f;

        float rawMin =
            center - displaySpan / 2.0f;

        float rawMax =
            center + displaySpan / 2.0f;

        displayMin =
            floorf(rawMin / SCALE_INCREMENT) *
            SCALE_INCREMENT;

        displayMax =
            ceilf(rawMax / SCALE_INCREMENT) *
            SCALE_INCREMENT;
    }

    struct tm timeInfo;
    char timeBuffer[16] = "--:--";

    if (getLocalTime(&timeInfo, 0))
    {
        strftime(
            timeBuffer,
            sizeof(timeBuffer),
            "%I:%M %p",
            &timeInfo
        );
    }

    String json;
    json.reserve(256);

    json += "{";

    json += "\"large\":";
    json += isnan(large) ? "null" : String(large, 1);

    json += ",\"small\":";
    json += isnan(small) ? "null" : String(small, 1);

    json += ",\"outdoor\":";
    json += isnan(outdoor) ? "null" : String(outdoor, 1);

    json += ",\"scaleMin\":";
    json += String(displayMin, 1);

    json += ",\"scaleMax\":";
    json += String(displayMax, 1);

    json += ",\"rssi\":";
    json += String(WiFi.RSSI());

    json += ",\"time\":\"";
    json += timeBuffer;
    json += "\"";

    json += "}";

    return json;
}