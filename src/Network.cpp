#include "Network.h"

void Network::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    loadCredentials();

    if (!_configuredSSID.isEmpty())
    {
        Serial.printf("Connecting to saved Wi-Fi network: %s\n",
                      _configuredSSID.c_str());

        connect(_configuredSSID, _configuredPassword);
    }
    else
    {
        Serial.println("No saved Wi-Fi credentials");
        _state = State::Disconnected;
    }
}

void Network::update()
{
    updateConnection();
    updateScan();
}

void Network::updateConnection()
{
    wl_status_t wifiStatus = WiFi.status();

    if (wifiStatus == WL_CONNECTED)
    {
        _state = State::Connected;
        return;
    }

    if (_state != State::Connecting)
    {
        if (_state == State::Connected)
            _state = State::Disconnected;

        return;
    }

    if (millis() - _connectionStart >= CONNECTION_TIMEOUT_MS)
    {
        WiFi.disconnect();
        _state = State::Failed;
    }
}

void Network::startScan()
{
    if (_scanState == ScanState::Scanning)
        return;

    clearScan();

    int result = WiFi.scanNetworks(true);

    if (result == WIFI_SCAN_FAILED)
    {
        _scanState = ScanState::Failed;
        return;
    }

    _scanState = ScanState::Scanning;
}

void Network::updateScan()
{
    if (_scanState != ScanState::Scanning)
        return;

    int result = WiFi.scanComplete();

    if (result == WIFI_SCAN_RUNNING)
        return;

    if (result == WIFI_SCAN_FAILED)
    {
        _scanState = ScanState::Failed;
        _scanCount = 0;
        return;
    }

    buildScanResults(result);
    _scanState = ScanState::Complete;
}

void Network::clearScan()
{
    WiFi.scanDelete();

    _scanCount = 0;
    _scanState = ScanState::Idle;
}

void Network::connect(const String& ssid, const String& password)
{
    if (ssid.isEmpty())
        return;

    WiFi.begin(ssid.c_str(), password.c_str());

    _connectionStart = millis();
    _state = State::Connecting;
}

void Network::disconnect()
{
    WiFi.disconnect();

    _state = State::Disconnected;
}

String Network::ssid() const
{
    if (WiFi.status() != WL_CONNECTED)
        return String();

    return WiFi.SSID();
}

int32_t Network::rssi() const
{
    if (WiFi.status() != WL_CONNECTED)
        return 0;

    return WiFi.RSSI();
}

String Network::scanSSID(int index) const
{
    if (_scanState != ScanState::Complete || index < 0 || index >= _scanCount)
        return String();

    return _scanResults[index].ssid;
}

int32_t Network::scanRSSI(int index) const
{
    if (_scanState != ScanState::Complete || index < 0 || index >= _scanCount)
        return 0;

    return _scanResults[index].rssi;
}

bool Network::scanEncrypted(int index) const
{
    if (_scanState != ScanState::Complete || index < 0 || index >= _scanCount)
        return false;

    return _scanResults[index].encrypted;
}

void Network::buildScanResults(int wifiScanCount)
{
    _scanCount = 0;

    for (int i = 0; i < wifiScanCount; i++)
    {
        String ssid = WiFi.SSID(i);

        if (ssid.isEmpty())
            continue;

        int32_t rssi = WiFi.RSSI(i);
        bool encrypted = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;

        int existingIndex = -1;

        for (int j = 0; j < _scanCount; j++)
        {
            if (_scanResults[j].ssid == ssid)
            {
                existingIndex = j;
                break;
            }
        }

        if (existingIndex >= 0)
        {
            //
            // Multiple access points are advertising the same SSID.
            // Keep the strongest one for display purposes.
            //

            if (rssi > _scanResults[existingIndex].rssi)
            {
                _scanResults[existingIndex].rssi = rssi;
                _scanResults[existingIndex].encrypted = encrypted;
            }

            continue;
        }

        if (_scanCount >= MAX_SCAN_RESULTS)
            continue;

        _scanResults[_scanCount].ssid = ssid;
        _scanResults[_scanCount].rssi = rssi;
        _scanResults[_scanCount].encrypted = encrypted;

        _scanCount++;
    }
}
String Network::macAddress() const
{
    return WiFi.macAddress();
}

String Network::ipAddress() const
{
    if (WiFi.status() != WL_CONNECTED)
        return String();

    return WiFi.localIP().toString();
}

void Network::setCredentials(const String& ssid, const String& password)
{
    _configuredSSID = ssid;
    _configuredPassword = password;

    Preferences prefs;

    if (!prefs.begin("network", false))
        return;

    prefs.putString("ssid", _configuredSSID);
    prefs.putString("password", _configuredPassword);

    prefs.end();
}
void Network::loadCredentials()
{
    Preferences prefs;

    if (!prefs.begin("network", true))
    {
        _configuredSSID = "";
        _configuredPassword = "";
        return;
    }

    _configuredSSID = prefs.getString("ssid", "");
    _configuredPassword = prefs.getString("password", "");

    prefs.end();
}