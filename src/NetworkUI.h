#pragma once

#include <lvgl.h>

#include "ScreenUI.h"
#include "Network.h"

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

    lv_obj_t* _selectButton = nullptr;
    lv_obj_t* _selectButtonLabel = nullptr;

    lv_obj_t* _networkList = nullptr;

    Network::ScanState _lastScanState = Network::ScanState::Idle;

    void showNetworkList();
    void hideNetworkList();
    void populateNetworkList();

    static void selectNetworkClicked(lv_event_t* event);
    static void networkClicked(lv_event_t* event);
    static void hiddenNetworkClicked(lv_event_t* event);
};