#pragma once

#include <WebServer.h>

#include "Status.h"
#include "StatusWeb.h"
#include "History.h"

class WebManager
{
public:
    WebManager(Status& status, History& history);

    void begin();
    void update();

private:
    WebServer _server{80};

    StatusWeb _statusWeb;
    History& _history;

    void sendFileList();
    void downloadFile();
    void deleteFile();
};