#include "OutdoorWeather.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>
namespace {
bool getJson(const String& url, JsonDocument& doc) {
    WiFiClientSecure client;
    client.setInsecure(); // TODO: use certificate validation in production.
    HTTPClient http;
    http.setTimeout(6000);
    if (!http.begin(client, url)) {
        Serial.printf("Weather HTTP begin failed: %s\\n", url.c_str());
        return false;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("Weather HTTP error %d: %s\\n", code, url.c_str());
        http.end();
        return false;
    }
    // Parse the response body, not the underlying HTTP stream.
    String payload = http.getString();
    http.end();
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("Weather JSON error: %s (length=%u)\\n", error.c_str(),
                      static_cast<unsigned>(payload.length()));
        Serial.printf("Weather response: %.200s\\n", payload.c_str());
        return false;
    }
    if (doc["error"].as<bool>()) {
        Serial.printf("Weather API error: %s\\n", doc["reason"].as<const char*>() | "unknown");
        return false;
    }
    return true;
}
}
bool OutdoorWeather::resolveZip(const String& zipcode) {
    JsonDocument doc;
    if (!getJson("https://api.zippopotam.us/us/" + zipcode, doc)) return false;
    JsonArray places = doc["places"].as<JsonArray>();
    if (places.isNull() || places.size() == 0) return false;
    _lat = String(places[0]["latitude"].as<const char*>()).toFloat();
    _lon = String(places[0]["longitude"].as<const char*>()).toFloat();
    _resolvedZip = zipcode;
    Serial.printf("Weather ZIP %s -> %.5f, %.5f\\n", zipcode.c_str(), _lat, _lon);
    return isfinite(_lat) && isfinite(_lon);
}
bool OutdoorWeather::fetchTemperature() {
    JsonDocument doc;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(_lat, 5)
       + "&longitude=" + String(_lon, 5) + "&current=temperature_2m&temperature_unit=celsius";
    if (!getJson(url, doc)) return false;
    JsonVariant temp = doc["current"]["temperature_2m"];
    if (temp.isNull()) {
        Serial.println("Weather response missing current.temperature_2m");
        return false;
    }
    _temperatureC = temp.as<float>();
    Serial.printf("Weather outdoor temperature: %.2f C\\n", _temperatureC);
    return isfinite(_temperatureC);
}
void OutdoorWeather::update(const String& zipcode, bool connected) {
    if (zipcode != _requestedZip) {
        _requestedZip = zipcode;
        _temperatureC = NAN;
        _lat = _lon = NAN;
        _resolvedZip = "";
        _attempted = false;
    }
    if (!connected || zipcode.length() != 5) return;
    uint32_t now = millis();
    if (_attempted && now - _lastAttempt < (isfinite(_temperatureC) ? 900000UL : 60000UL)) return;
    _attempted = true;
    _lastAttempt = now;
    if (_resolvedZip != zipcode && !resolveZip(zipcode)) {
        Serial.printf("Weather ZIP lookup failed: %s\\n", zipcode.c_str());
        return;
    }
    if (!fetchTemperature()) Serial.println("Weather temperature fetch/parse failed");
}
