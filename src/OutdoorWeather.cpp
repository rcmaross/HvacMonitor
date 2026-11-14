#include "OutdoorWeather.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>
namespace {
bool getJson(const String& url, JsonDocument& doc) {
    WiFiClientSecure client;
    client.setInsecure(); // TODO: replace with CA validation before production.
    HTTPClient http;
    http.setTimeout(4000);
    if (!http.begin(client, url)) return false;
    int code = http.GET();
    bool ok = code == HTTP_CODE_OK && deserializeJson(doc, http.getStream()) == DeserializationError::Ok;
    http.end();
    return ok;
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
    return isfinite(_lat) && isfinite(_lon);
}
bool OutdoorWeather::fetchTemperature() {
    JsonDocument doc;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(_lat, 5)
       + "&longitude=" + String(_lon, 5) + "&current=temperature_2m&temperature_unit=celsius";
    if (!getJson(url, doc)) return false;
    if (!doc["current"]["temperature_2m"].is<float>()) return false;
    _temperatureC = doc["current"]["temperature_2m"].as<float>();
    return isfinite(_temperatureC);
}
void OutdoorWeather::update(const String& zipcode, bool connected) {
    if (zipcode != _resolvedZip) {
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
    if (_resolvedZip != zipcode && !resolveZip(zipcode)) return;
    fetchTemperature();
}
