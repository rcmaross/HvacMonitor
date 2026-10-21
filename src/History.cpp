#include "History.h"
#include "App.h"
#include "Analysis.h"
#include "BuildInfo.h"

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

    static constexpr int CSV_VERSION = 6;

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
            "outside_slope,"
            "large_outside_slope,"
            "compressor_state,"
            "analysis_sample_count,"
            "analysis_avg_delta_c,"
            "analysis_end_reason,"
            "firmware_version"
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
    const float outsideSlope = _status.outsideSlopeCPerMin() * slopeFactor;
    const float largeOutsideSlope = _status.largeOutsideSlopeCPerMin() * slopeFactor;

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

    const auto& completed = analysis.completed();
    // MINUTE contains the boundary sample; STATE_CHANGE marks the new state.
    // On STATE_CHANGE the prior row holds the old interval's final average.
    const bool minute = completed.valid && strcmp(completed.reason, "MINUTE") == 0;
    const char* reason = completed.valid ? completed.reason : "IN_PROGRESS";
    const unsigned count = minute ? (unsigned)completed.sampleCount
                                  : (unsigned)analysis.sampleCount();
    const float average = minute ? completed.avgDeltaC : analysis.runningAverage();

    file.printf(
        "%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%.4f,%.4f,%s,%u,%.3f,%s,%s\n",
        timestamp, large, small, outside,
        fabsf(large - small), fabsf(large - outside), fabsf(small - outside),
        largeSlope, smallSlope, largeSlope - smallSlope,
        outsideSlope, largeOutsideSlope,
        Status::compressorStateName(_status.compressorState()),
        count, average, reason, BuildInfo::version()
    );

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
bool History::selectAnalysisFiles(uint32_t now) {
    _analysisFileCount = 0;
    _analysisFileIndex = 0;
    _analysisFileEnd = 0;
    if (!_available || now < 1700000000) return false;
    ScopedMount mount;
    if (!mount.isReady()) return false;
    File directory = SD.open(DIRECTORY);
    if (!directory || !directory.isDirectory()) return false;
    File file = directory.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String name = file.name();
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);
            if (name.startsWith("history_") && name.endsWith(".csv")) {
                time_t modified = file.getLastWrite();
                if (modified >= 1700000000 && modified <= (time_t)now &&
                    (uint32_t)(now - modified) <= 86400) {
                    // Keep the most recently modified MAX_ANALYSIS_FILES if necessary.
                    size_t slot = _analysisFileCount;
                    if (slot == MAX_ANALYSIS_FILES) {
                        slot = 0;
                        for (size_t j = 1; j < _analysisFileCount; ++j)
                            if (_analysisModified[j] < _analysisModified[slot]) slot = j;
                        if (modified <= _analysisModified[slot]) {
                            file.close();
                            file = directory.openNextFile();
                            continue;
                        }
                    } else ++_analysisFileCount;
                    _analysisFiles[slot] = name;
                    _analysisModified[slot] = modified;
                }
            }
        }
        file.close();
        file = directory.openNextFile();
    }
    directory.close();
    // Oldest modification first, tie-break by filename for deterministic order.
    for (size_t i = 1; i < _analysisFileCount; ++i) {
        for (size_t j = i; j > 0; --j) {
            if (_analysisModified[j - 1] < _analysisModified[j] ||
                (_analysisModified[j - 1] == _analysisModified[j] &&
                 _analysisFiles[j - 1] <= _analysisFiles[j])) break;
            time_t tempTime = _analysisModified[j - 1];
            _analysisModified[j - 1] = _analysisModified[j];
            _analysisModified[j] = tempTime;
            String tempName = _analysisFiles[j - 1];
            _analysisFiles[j - 1] = _analysisFiles[j];
            _analysisFiles[j] = tempName;
        }
    }
    Serial.printf("Analysis: %u eligible history files\n", (unsigned)_analysisFileCount);
    for (size_t i = 0; i < _analysisFileCount; ++i)
        Serial.printf("Analysis: file %u: %s (modified %ld)\n",
                      (unsigned)i, _analysisFiles[i].c_str(), (long)_analysisModified[i]);
    return _analysisFileCount > 0;
}

bool History::readAnalysisBatch(uint32_t& cursor, size_t limit,
                                const std::function<void(const String&)>& callback, bool& done) {
    done = true;
    if (!_available || !_analysisFileCount) return false;
    ScopedMount mount;
    if (!mount.isReady()) return false;
    size_t processed = 0;
    while (_analysisFileIndex < _analysisFileCount && processed < limit) {
        File file = SD.open(makePath(_analysisFiles[_analysisFileIndex]), FILE_READ);
        if (!file) {
            Serial.printf("Analysis: cannot open %s\n", _analysisFiles[_analysisFileIndex].c_str());
            ++_analysisFileIndex;
            cursor = 0;
            continue;
        }
        // Snapshot the file length when first opened. New writes during rebuild
        // are handled by the live path after the startup cutoff.
        if (!cursor) _analysisFileEnd = file.size();
        if (!file.seek(cursor)) {
            file.close();
            ++_analysisFileIndex;
            cursor = 0;
            continue;
        }
        while (file.available() && file.position() < _analysisFileEnd && processed < limit) {
            String line = file.readStringUntil('\n');
            if (file.position() > _analysisFileEnd) break;
            callback(line);
            ++processed;
        }
        cursor = file.position();
        bool fileDone = cursor >= _analysisFileEnd || !file.available();
        file.close();
        if (fileDone) {
            ++_analysisFileIndex;
            cursor = 0;
        }
    }
    done = _analysisFileIndex >= _analysisFileCount;
    return true;
}
