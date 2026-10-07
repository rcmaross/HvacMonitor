#pragma once

#include <WebServer.h>
#include "Status.h"
#include "StatusWeb.h"

class WebManager
{
public:
    WebManager(Status& status);

    void begin();
    void update();

private:
    WebServer _server{80};
    StatusWeb _statusWeb;
};