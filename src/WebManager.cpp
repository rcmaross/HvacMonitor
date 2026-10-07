#include "WebManager.h"
#include "generated/index_html.h"
#include "generated/status_html.h"

WebManager::WebManager(Status& status)
    : _statusWeb(status)
{
}

void WebManager::begin()
{
    _server.on("/", HTTP_GET, [this]()
    {
        _server.send(200, "text/html", INDEX_HTML);
    });

    _server.on("/status", HTTP_GET, [this]()
    {
        _server.send(200, "text/html", STATUS_HTML);
    });

    _server.on("/api/status", HTTP_GET, [this]()
    {
        _server.send(200, "application/json", _statusWeb.statusJson());
    });
    
    _server.onNotFound([this]()
    {
        Serial.printf(
            "HTTP 404: %s\n",
            _server.uri().c_str()
        );

        _server.send(
            404,
            "text/plain",
            "Not found"
        );
    });

    _server.begin();

    Serial.println("Web server started");
}

void WebManager::update()
{
    _server.handleClient();
}