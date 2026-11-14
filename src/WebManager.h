#pragma once

#include <WebServer.h>

#include "Status.h"
#include "StatusWeb.h"
#include "History.h"
#include "Analysis.h"
#include "Settings.h"

class WebManager
{
public:
    WebManager(Status& status, History& history, Analysis& analysis, Settings& settings);

    void begin();
    void update();

private:
    WebServer _server{80};

    StatusWeb _statusWeb;
    History& _history;
    Analysis& _analysis;
    Settings& _settings;

    void sendFileList();
    void downloadFile();
    void deleteFile();
};