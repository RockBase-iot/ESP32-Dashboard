#pragma once

#include <stdint.h>

#include <string>
#include <vector>

struct TimezoneCatalogEntry {
    const char *iana;
    const char *posix;
    int standardOffsetSeconds;
    int daylightOffsetSeconds;
    bool observesDst;
};

const std::vector<TimezoneCatalogEntry> &timezoneCatalog();
std::string canonicalTimezoneId(const std::string &timezoneId);
std::string timezonePosixRule(const std::string &iana);
int timezoneOffsetSecondsAtUtc(const std::string &iana, int64_t utcSeconds);
