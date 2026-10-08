#include "WebManager.h"
#include "generated/index_html.h"
#include "generated/status_html.h"
#include "generated/history_html.h"

static bool validHistoryFilename(const String& name)
{
    if (name.length() == 0)
        return false;

    if (name.indexOf('/') >= 0)
        return false;

    if (name.indexOf('\\') >= 0)
        return false;

    if (name == "." || name == "..")
        return false;

    return true;
}

WebManager::WebManager(Status& status, History& history)
    : _statusWeb(status),
      _history(history)
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

    _server.on("/history", HTTP_GET, [this]()
    {
        _server.send(200, "text/html", HISTORY_HTML);
    });
    _server.on("/api/history/files", HTTP_GET, [this]()
    {
        sendFileList();
    });

    _server.on("/api/history/download", HTTP_GET, [this]()
    {
        downloadFile();
    });

    _server.on("/api/history/delete", HTTP_POST, [this]()
    {
        deleteFile();
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

void WebManager::sendFileList()
{
    if (!_history.available())
    {
        _server.send(503, "application/json", "[]");
        return;
    }

    _server.send(
        200,
        "application/json",
        _history.fileListJson()
    );
}

void WebManager::downloadFile()
{
    if (!_history.available())
    {
        _server.send(503, "text/plain", "SD card unavailable");
        return;
    }

    if (!_server.hasArg("name"))
    {
        _server.send(400, "text/plain", "Missing filename");
        return;
    }

    String name = _server.arg("name");

    if (!validHistoryFilename(name))
    {
        _server.send(400, "text/plain", "Invalid filename");
        return;
    }

    auto stream = _history.openFileReadStream(name);

    if (!stream.isOpen())
    {
        _server.send(404, "text/plain", "File not found");
        return;
    }

    _server.sendHeader(
        "Content-Disposition",
        "attachment; filename=\"" + name + "\""
    );

    _server.streamFile(
        stream.getFile(),
        "application/octet-stream"
    );
}
void WebManager::deleteFile()
{
    if (!_history.available())
    {
        _server.send(503, "text/plain", "SD card unavailable");
        return;
    }

    if (!_server.hasArg("name"))
    {
        _server.send(400, "text/plain", "Missing filename");
        return;
    }

    String name = _server.arg("name");

    if (!validHistoryFilename(name))
    {
        _server.send(400, "text/plain", "Invalid filename");
        return;
    }

    if (!_history.deleteFile(name))
    {
        _server.send(404, "text/plain", "File not found");
        return;
    }

    _server.send(200, "text/plain", "OK");
}