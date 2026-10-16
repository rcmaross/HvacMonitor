#pragma once

#include <WebServer.h>

#include "Status.h"
#include "StatusWeb.h"
#include "History.h"
#include "Analysis.h"

class WebManager
{
public:
    WebManager(Status& status, History& history, Analysis& analysis);

    void begin();
    void update();

private:
    WebServer _server{80};

    StatusWeb _statusWeb;
    History& _history;
    Analysis& _analysis;

    void sendFileList();
    void downloadFile();
    void deleteFile();
};