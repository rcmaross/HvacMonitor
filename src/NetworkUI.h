#pragma once

#include <lvgl.h>

#include "ScreenUI.h"
#include "Network.h"
#include "TextEntryUI.h"

class NetworkUI : public ScreenUI
{
public:
    NetworkUI(lv_obj_t* parent, Network& network);
    virtual ~NetworkUI() override;

    void update() override;

private:
    Network& _network;

    lv_obj_t* _root = nullptr;

    lv_obj_t* _statusLabel = nullptr;
    lv_obj_t* _ssidLabel = nullptr;
    lv_obj_t* _rssiLabel = nullptr;
    lv_obj_t* _macLabel = nullptr;
    lv_obj_t* _ipLabel = nullptr;

    lv_obj_t* _selectButton = nullptr;
    lv_obj_t* _selectButtonLabel = nullptr;

    lv_obj_t* _networkList = nullptr;

    lv_obj_t* _connectButton = nullptr;
    lv_obj_t* _connectButtonLabel = nullptr;

    Network::ScanState _lastScanState = Network::ScanState::Idle;

    String _selectedSSID;
    String _password;

    TextEntryUI* _textEntry = nullptr;
    void showNetworkList();
    void hideNetworkList();
    void populateNetworkList();
    void showPasswordEntry();
    void closeTextEntry();

    static void passwordEntered(const String& text, void* userData);
    static void passwordCancelled(void* userData);
    static void selectNetworkClicked(lv_event_t* event);
    static void networkClicked(lv_event_t* event);
    static void hiddenNetworkClicked(lv_event_t* event);
    static void backFromNetworkListClicked(lv_event_t* event);
    static void connectClicked(lv_event_t* event);
};