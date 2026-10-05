#include "Network.h"

void Network::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    if (WiFi.status() == WL_CONNECTED)
        _state = State::Connected;
    else
        _state = State::Disconnected;
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

    _scanCount = result;
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

    return WiFi.SSID(index);
}

int32_t Network::scanRSSI(int index) const
{
    if (_scanState != ScanState::Complete || index < 0 || index >= _scanCount)
        return 0;

    return WiFi.RSSI(index);
}

bool Network::scanEncrypted(int index) const
{
    if (_scanState != ScanState::Complete || index < 0 || index >= _scanCount)
        return false;

    return WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
}