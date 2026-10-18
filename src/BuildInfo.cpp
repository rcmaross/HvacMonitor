#include "BuildInfo.h"
#include "generated/build_info.h"
namespace BuildInfo {
const char* version() { return HVAC_FW_VERSION; }
const char* commit() { return HVAC_FW_SHA; }
const char* buildType() { return HVAC_FW_TYPE; }
const char* buildDate() { return HVAC_FW_DATE; }
}
