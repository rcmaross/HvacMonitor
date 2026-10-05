#include "NetworkUI.h"

namespace
{
    const lv_color_t BACKGROUND = lv_color_hex(0x101418);
    const lv_color_t PRIMARY_TEXT = lv_color_hex(0xF2F4F5);
    const lv_color_t SECONDARY_TEXT = lv_color_hex(0xAEB7BF);

    constexpr int LEFT_MARGIN = 8;
    constexpr int RIGHT_MARGIN = 8;
}

NetworkUI::NetworkUI(lv_obj_t* parent, Network& network)
    : _network(network)
{
    _root = lv_obj_create(parent);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, BACKGROUND, 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_root, 0, 0);
    lv_obj_set_style_pad_all(_root, 0, 0);
    lv_obj_set_scrollable(_root, false);

    //
    // Connection status
    //

    _statusLabel = lv_label_create(_root);
    lv_label_set_text(_statusLabel, "Wi-Fi: Disconnected");
    lv_obj_set_pos(_statusLabel, LEFT_MARGIN, 6);
    lv_obj_set_style_text_color(_statusLabel, PRIMARY_TEXT, 0);
    lv_obj_set_style_text_font(_statusLabel, &lv_font_montserrat_14, 0);

    //
    // Current SSID
    //

    _ssidLabel = lv_label_create(_root);
    lv_label_set_text(_ssidLabel, "SSID: --");
    lv_obj_set_pos(_ssidLabel, LEFT_MARGIN, 30);
    lv_obj_set_style_text_color(_ssidLabel, PRIMARY_TEXT, 0);
    lv_obj_set_style_text_font(_ssidLabel, &lv_font_montserrat_14, 0);

    //
    // RSSI
    //

    _rssiLabel = lv_label_create(_root);
    lv_label_set_text(_rssiLabel, "Signal: --");
    lv_obj_set_pos(_rssiLabel, LEFT_MARGIN, 54);
    lv_obj_set_style_text_color(_rssiLabel, SECONDARY_TEXT, 0);
    lv_obj_set_style_text_font(_rssiLabel, &lv_font_montserrat_14, 0);

    //
    // Select Network button
    //

    _selectButton = lv_button_create(_root);
    lv_obj_set_size(_selectButton, 160, 38);
    lv_obj_align(_selectButton, LV_ALIGN_BOTTOM_MID, 0, -12);

    _selectButtonLabel = lv_label_create(_selectButton);
    lv_label_set_text(_selectButtonLabel, "Select Network");
    lv_obj_center(_selectButtonLabel);

    lv_obj_add_event_cb(_selectButton, selectNetworkClicked, LV_EVENT_CLICKED, this);

    update();
}

NetworkUI::~NetworkUI()
{
    if (_root)
    {
        lv_obj_delete(_root);
        _root = nullptr;
    }
}

void NetworkUI::update()
{
    char buffer[96];

    //
    // Connection state
    //

    switch (_network.state())
    {
        case Network::State::Disconnected:
            lv_label_set_text(_statusLabel, "Wi-Fi: Disconnected");
            break;

        case Network::State::Connecting:
            lv_label_set_text(_statusLabel, "Wi-Fi: Connecting...");
            break;

        case Network::State::Connected:
            lv_label_set_text(_statusLabel, "Wi-Fi: Connected");
            break;

        case Network::State::Failed:
            lv_label_set_text(_statusLabel, "Wi-Fi: Connection failed");
            break;
    }

    //
    // Connected network
    //

    if (_network.isConnected())
    {
        snprintf(buffer, sizeof(buffer), "SSID: %s", _network.ssid().c_str());
        lv_label_set_text(_ssidLabel, buffer);

        snprintf(buffer, sizeof(buffer), "Signal: %d dBm", _network.rssi());
        lv_label_set_text(_rssiLabel, buffer);
    }
    else
    {
        lv_label_set_text(_ssidLabel, "SSID: --");
        lv_label_set_text(_rssiLabel, "Signal: --");
    }

    //
    // Detect completion of an asynchronous scan.
    //

    Network::ScanState scanState = _network.scanState();

    if (scanState != _lastScanState)
    {
        _lastScanState = scanState;

        if (scanState == Network::ScanState::Scanning)
        {
            lv_label_set_text(_selectButtonLabel, "Scanning...");
            lv_obj_add_state(_selectButton, LV_STATE_DISABLED);
        }
        else
        {
            lv_label_set_text(_selectButtonLabel, "Select Network");
            lv_obj_remove_state(_selectButton, LV_STATE_DISABLED);

            if (scanState == Network::ScanState::Complete)
                showNetworkList();
        }
    }
}

void NetworkUI::showNetworkList()
{
    populateNetworkList();
}

void NetworkUI::hideNetworkList()
{
    if (_networkList)
    {
        lv_obj_delete(_networkList);
        _networkList = nullptr;
    }
}

void NetworkUI::populateNetworkList()
{
    hideNetworkList();

    _networkList = lv_obj_create(_root);
    lv_obj_set_size(_networkList, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(_networkList, 0, 0);
    lv_obj_set_style_bg_color(_networkList, BACKGROUND, 0);
    lv_obj_set_style_bg_opa(_networkList, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_networkList, 0, 0);
    lv_obj_set_style_pad_all(_networkList, 4, 0);
    lv_obj_set_flex_flow(_networkList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(_networkList, LV_DIR_VER);

    int count = _network.scanCount();

    for (int i = 0; i < count; i++)
    {
        lv_obj_t* button = lv_button_create(_networkList);
        lv_obj_set_width(button, LV_PCT(100));
        lv_obj_set_height(button, 36);

        //
        // Store the scan-result index on the button itself.
        //

        lv_obj_set_user_data(button, reinterpret_cast<void*>(static_cast<intptr_t>(i)));

        lv_obj_t* label = lv_label_create(button);

        char buffer[96];
        snprintf(buffer, sizeof(buffer), "%s    %d dBm",
                 _network.scanSSID(i).c_str(), _network.scanRSSI(i));

        lv_label_set_text(label, buffer);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 4, 0);

        lv_obj_add_event_cb(button, networkClicked, LV_EVENT_CLICKED, this);
    }

    //
    // Always provide manual entry for hidden networks.
    //

    lv_obj_t* hiddenButton = lv_button_create(_networkList);
    lv_obj_set_width(hiddenButton, LV_PCT(100));
    lv_obj_set_height(hiddenButton, 36);

    lv_obj_t* hiddenLabel = lv_label_create(hiddenButton);
    lv_label_set_text(hiddenLabel, "Other / Hidden Network...");
    lv_obj_center(hiddenLabel);

    lv_obj_add_event_cb(hiddenButton, hiddenNetworkClicked, LV_EVENT_CLICKED, this);
}

void NetworkUI::selectNetworkClicked(lv_event_t* event)
{
    auto* ui = static_cast<NetworkUI*>(lv_event_get_user_data(event));

    if (!ui)
        return;

    ui->_network.startScan();
    ui->update();
}

void NetworkUI::networkClicked(lv_event_t* event)
{
    auto* ui = static_cast<NetworkUI*>(lv_event_get_user_data(event));
    lv_obj_t* button = static_cast<lv_obj_t*>(lv_event_get_target(event));

    if (!ui || !button)
        return;

    int index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(button)));

    String ssid = ui->_network.scanSSID(index);

    Serial.printf("Selected Wi-Fi network: %s\n", ssid.c_str());

    //
    // Next step:
    // open password editor for this SSID.
    //
}

void NetworkUI::hiddenNetworkClicked(lv_event_t* event)
{
    auto* ui = static_cast<NetworkUI*>(lv_event_get_user_data(event));

    if (!ui)
        return;

    Serial.println("Selected manual/hidden Wi-Fi network");

    //
    // Next step:
    // open full-screen SSID editor.
    //
}