#pragma once

#include <stdint.h>

#include <string>

struct LocalDateTime {
    int year = 1970;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

class TimezoneResolver {
public:
    int64_t toUtc(const std::string &timezoneId, const LocalDateTime &local) const;
    int64_t fromUtc(const std::string &timezoneId, int64_t utc) const;
    int offsetSeconds(const std::string &timezoneId, int64_t utc) const;
};
