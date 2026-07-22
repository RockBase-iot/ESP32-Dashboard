#include "web_config_validation.h"

#include <algorithm>
#include <cctype>

#include "app/calendar/calendar_source.h"

namespace {
std::string webConfigLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

bool contains(const std::string &text, const char *needle) {
    return webConfigLowerAscii(text).find(webConfigLowerAscii(needle)) != std::string::npos;
}
}  // namespace

std::string redactKeyValueForLog(const std::string &key, const std::string & /*value*/) {
    if (contains(key, "pswd") || contains(key, "password") || contains(key, "secret") ||
        contains(key, "token")) {
        return "<redacted>";
    }
    return "<configured>";
}

bool isAllowedRotationInterval(uint16_t minutes) {
    return minutes == 0 || minutes == 30 || minutes == 60 || minutes == 90 ||
           minutes == 120 || minutes == 150;
}

bool isSafeDashboardUrl(const std::string &url) {
    return normalizeCalendarSourceUrl(url).ok;
}

uint16_t normalizePortalWindowSec(int32_t seconds) {
    if (seconds <= 0) {
        return 0;
    }
    if (seconds > kPortalWindowSecMax) {
        return kPortalWindowSecMax;
    }
    return static_cast<uint16_t>(seconds);
}
