#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "Status.h"

class History
{
private:
    class ScopedMount
    {
    public:
        ScopedMount()
        {
            SD.end();
            delay(5);

            _mounted = SD.begin(
                4,
                SPI,
                4000000,
                "/sd",
                5,
                false
            );
        }

        ~ScopedMount()
        {
            SD.end();
        }

        bool isReady() const
        {
            return _mounted;
        }

    private:
        bool _mounted = false;
    };

public:
    class ActiveFileStream
    {
    public:
        explicit ActiveFileStream(const String& path)
            : _mount()
        {
            if (_mount.isReady())
                _file = SD.open(path, FILE_READ);
        }

        ~ActiveFileStream()
        {
            if (_file)
                _file.close();
        }

        bool isOpen() const
        {
            return (bool)_file;
        }

        File& getFile()
        {
            return _file;
        }

    private:
        ScopedMount _mount;
        File _file;
    };

    explicit History(Status& status);

    void begin();
    void write();

    bool available() const
    {
        return _available;
    }

    String fileListJson();
    bool deleteFile(const String& filename);
    ActiveFileStream openFileReadStream(const String& filename);

private:
    Status& _status;

    bool _available = false;

    static constexpr const char* DIRECTORY = "/HvacMonitor";

    String makePath(const String& filename) const;
};