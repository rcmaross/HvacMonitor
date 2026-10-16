#include "History.h"
#include "App.h"
#include "Analysis.h"

#include <SD.h>
#include <math.h>
#include <time.h>

History::History(Status& status)
    : _status(status)
{
}

void History::begin()
{
    Serial.println("History: initializing SD card");

    ScopedMount mount;

    if (!mount.isReady())
    {
        Serial.println("ERROR: SD card initialization failed");
        return;
    }

    if (!SD.exists(DIRECTORY))
    {
        if (!SD.mkdir(DIRECTORY))
        {
            Serial.printf("ERROR: Could not create %s\n", DIRECTORY);
            return;
        }

        Serial.printf("History: created %s\n", DIRECTORY);
    }

    _available = true;

    Serial.println("History SD card ready");
}

void History::write(const Analysis& analysis)
{
    if (!_available)
        return;

    ScopedMount mount;

    if (!mount.isReady())
    {
        Serial.println("ERROR: SD card mount failed during history write");
        return;
    }

    static constexpr int CSV_VERSION = 4;

    char filename[64];

    snprintf(
        filename,
        sizeof(filename),
        "%s/history_v%d.csv",
        DIRECTORY,
        CSV_VERSION
    );

    bool newFile = !SD.exists(filename);

    File file = SD.open(filename, FILE_APPEND);

    if (!file)
    {
        Serial.printf("ERROR: Could not open %s\n", filename);
        return;
    }

    if (newFile)
    {
        file.println(
            "timestamp,"
            "large,"
            "small,"
            "outside,"
            "large_small_delta,"
            "large_outside_delta,"
            "small_outside_delta,"
            "large_slope,"
            "small_slope,"
            "large_small_slope,"
            "compressor_state,"
            "analysis_sample_count,"
            "analysis_avg_delta_c,"
            "analysis_end_reason"
        );
    }

    float large = app.convertTempFromCelsius(_status.largePipeTemperature());
    float small = app.convertTempFromCelsius(_status.smallPipeTemperature());
    float outside = app.convertTempFromCelsius(_status.outdoorTemperature());
    
    // Status computes slopes in C/min; convert only for CSV output.
    const float slopeFactor = app.convertTempFromCelsius(1.0f) -
                              app.convertTempFromCelsius(0.0f);
    const float largeSlope = _status.largeSlopeCPerMin() * slopeFactor;
    const float smallSlope = _status.smallSlopeCPerMin() * slopeFactor;

    time_t now = time(nullptr);

    struct tm local;
    localtime_r(&now, &local);

    char timestamp[32];

    strftime(
        timestamp,
        sizeof(timestamp),
        "%Y-%m-%d %H:%M:%S",
        &local
    );

    file.printf(
        "%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%s\n",
        timestamp,
        large,
        small,
        outside,
        fabsf(large - small),
        fabsf(large - outside),
        fabsf(small - outside),
        largeSlope,
        smallSlope,
        largeSlope - smallSlope,
        Status::compressorStateName(_status.compressorState()),
        (unsigned)analysis.sampleCount(),
        analysis.runningAverage()
    );
    const auto& completed = analysis.completed();
    if (completed.valid) {
        // Summary rows carry no raw sensor values and are not replayed as samples.
        file.printf("%s,,,,,,,,,,%s,%u,%.3f,%s\\n",
                    timestamp,
                    completed.type == AnalysisEventType::OFF ? "OFF" : "HEATING",
                    (unsigned)completed.sampleCount,
                    completed.avgDeltaC,
                    completed.reason);
    }

    file.flush();
    file.close();
}

String History::makePath(const String& filename) const
{
    return String(DIRECTORY) + "/" + filename;
}
String History::fileListJson()
{
    if (!_available)
        return "[]";

    ScopedMount mount;

    if (!mount.isReady())
    {
        Serial.println("ERROR: SD card mount failed while listing files");
        return "[]";
    }

    File directory = SD.open(DIRECTORY);

    if (!directory || !directory.isDirectory())
    {
        Serial.printf("ERROR: Could not open %s\n", DIRECTORY);

        if (directory)
            directory.close();

        return "[]";
    }

    String json = "[";
    bool first = true;

    File file = directory.openNextFile();

    while (file)
    {
        if (!file.isDirectory())
        {
            String name = file.name();

            // Some FS implementations return the complete path.
            int slash = name.lastIndexOf('/');

            if (slash >= 0)
                name = name.substring(slash + 1);

            if (!first)
                json += ",";

            json += "{\"name\":\"";
            json += name;
            json += "\",\"size\":";
            json += String(file.size());
            json += "}";

            first = false;
        }

        file.close();
        file = directory.openNextFile();
    }

    directory.close();

    json += "]";

    return json;
}
bool History::deleteFile(const String& filename)
{
    if (!_available)
        return false;

    ScopedMount mount;

    if (!mount.isReady())
    {
        Serial.println("ERROR: SD card mount failed while deleting file");
        return false;
    }

    String path = makePath(filename);

    if (!SD.exists(path))
        return false;

    bool result = SD.remove(path);

    if (result)
        Serial.printf("History: deleted %s\n", path.c_str());

    return result;
}

History::ActiveFileStream History::openFileReadStream(
    const String& filename
)
{
    return ActiveFileStream(makePath(filename));
}
bool History::csvMetric() const {
    // The existing CSV has no per-row units metadata.
    return app.settingsMetric();
}
bool History::selectAnalysisFile(uint32_t now) {
    _analysisFile = "";
    if (!_available || now < 1700000000) return false;
    ScopedMount mount;
    if (!mount.isReady()) return false;
    File directory = SD.open(DIRECTORY);
    if (!directory || !directory.isDirectory()) return false;
    time_t newest = 0;
    File file = directory.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);
            if (name.startsWith("history_") && name.endsWith(".csv")) {
                time_t modified = file.getLastWrite();
                if (modified >= 1700000000 && modified <= (time_t)now &&
                    modified > newest) {
                    newest = modified;
                    _analysisFile = name;
                }
            }
        }
        file.close();
        file = directory.openNextFile();
    }
    directory.close();
    if (!newest || (uint32_t)(now - newest) > 86400) {
        _analysisFile = "";
        return false;
    }
    Serial.printf("Analysis: selected %s (modified %ld)\\n",
                  _analysisFile.c_str(), (long)newest);
    return true;
}

bool History::readAnalysisBatch(uint32_t& cursor, size_t limit,
                                const std::function<void(const String&)>& callback, bool& done) {
    done = true;
    if (!_available || !_analysisFile.length()) return false;
    ScopedMount mount;
    if (!mount.isReady()) return false;
    File file = SD.open(makePath(_analysisFile), FILE_READ);
    if (!file || !file.seek(cursor)) return false;
    size_t count = 0;
    while (file.available() && count < limit) {
        String line = file.readStringUntil('\n');
        callback(line);
        ++count;
    }
    cursor = file.position();
    done = !file.available();
    file.close();
    return true;
}
