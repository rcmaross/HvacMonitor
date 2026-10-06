#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

class Network
{
public:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
        Failed
    };

    enum class ScanState
    {
        Idle,
        Scanning,
        Complete,
        Failed
    };

    void begin();
    void update();

    void startScan();
    void clearScan();

    void connect(const String& ssid, const String& password);
    void disconnect();

    State state() const { return _state; }
    ScanState scanState() const { return _scanState; }

    bool isConnected() const { return _state == State::Connected; }

    String ssid() const;
    int32_t rssi() const;
    String macAddress() const;
    String ipAddress() const;

    int scanCount() const { return _scanCount; }
    String scanSSID(int index) const;
    int32_t scanRSSI(int index) const;
    bool scanEncrypted(int index) const;

    void setCredentials(const String& ssid, const String& password);

    const String& configuredSSID() const { return _configuredSSID; }

private:
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 15000;
    static constexpr int MAX_SCAN_RESULTS = 20;

    struct ScanResult
    {
        String ssid;
        int32_t rssi;
        bool encrypted;
    };


    State _state = State::Disconnected;
    ScanState _scanState = ScanState::Idle;

    ScanResult _scanResults[MAX_SCAN_RESULTS];
    int _scanCount = 0;

    uint32_t _connectionStart = 0;
    String _configuredSSID;
    String _configuredPassword;

    void updateConnection();
    void updateScan();
    void buildScanResults(int wifiScanCount);
    void loadCredentials(); 
};