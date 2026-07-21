#include "timezone_catalog.h"

#include <algorithm>

namespace {
int64_t timezoneCatalogDaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

void timezoneCatalogCivilFromDays(int64_t z, int &year, unsigned &month, unsigned &day) {
    z += 719468LL;
    const int era = static_cast<int>((z >= 0 ? z : z - 146096) / 146097);
    const unsigned doe = static_cast<unsigned>(z - static_cast<int64_t>(era) * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp + (mp < 10 ? 3 : static_cast<unsigned>(-9));
    year += (month <= 2);
}

int weekday(int year, int month, int day) {
    const int64_t epochDays = timezoneCatalogDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    int dow = static_cast<int>((epochDays + 4) % 7);
    return dow < 0 ? dow + 7 : dow;
}

int nthSunday(int year, int month, int ordinal) {
    const int firstDow = weekday(year, month, 1);
    const int firstSunday = 1 + ((7 - firstDow) % 7);
    return firstSunday + (ordinal - 1) * 7;
}

int lastSunday(int year, int month) {
    static const int monthDays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int days = monthDays[month];
    const bool leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    if (month == 2 && leap) {
        ++days;
    }
    return days - weekday(year, month, days);
}

bool inNewYorkDstUtc(int64_t utcSeconds) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    timezoneCatalogCivilFromDays(utcSeconds / 86400LL, year, month, day);
    const int startDay = nthSunday(year, 3, 2);
    const int endDay = nthSunday(year, 11, 1);
    const int64_t start = timezoneCatalogDaysFromCivil(year, 3, static_cast<unsigned>(startDay)) * 86400LL + 7 * 3600LL;
    const int64_t end = timezoneCatalogDaysFromCivil(year, 11, static_cast<unsigned>(endDay)) * 86400LL + 6 * 3600LL;
    return utcSeconds >= start && utcSeconds < end;
}

bool inEuropeDstUtc(const std::string &iana, int64_t utcSeconds) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    timezoneCatalogCivilFromDays(utcSeconds / 86400LL, year, month, day);
    const int startDay = lastSunday(year, 3);
    const int endDay = lastSunday(year, 10);
    const int64_t start = timezoneCatalogDaysFromCivil(year, 3, static_cast<unsigned>(startDay)) * 86400LL + 1 * 3600LL;
    const int64_t end = timezoneCatalogDaysFromCivil(year, 10, static_cast<unsigned>(endDay)) * 86400LL + 1 * 3600LL;
    (void)iana;
    return utcSeconds >= start && utcSeconds < end;
}
}  // namespace

const std::vector<TimezoneCatalogEntry> &timezoneCatalog() {
    static const std::vector<TimezoneCatalogEntry> kCatalog = {
        {"Etc/UTC", "UTC0", 0, 0, false},
        {"UTC", "UTC0", 0, 0, false},
        {"GMT", "UTC0", 0, 0, false},
        {"Asia/Shanghai", "CST-8", 8 * 3600, 8 * 3600, false},
        {"Asia/Tokyo", "JST-9", 9 * 3600, 9 * 3600, false},
        {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0/2", 0, 3600, true},
        {"Europe/Berlin", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2", -5 * 3600, -4 * 3600, true},
        {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2", -6 * 3600, -5 * 3600, true},
        {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2", -7 * 3600, -6 * 3600, true},
        {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2", -8 * 3600, -7 * 3600, true},
    };
    return kCatalog;
}

std::string timezonePosixRule(const std::string &iana) {
    for (const TimezoneCatalogEntry &entry : timezoneCatalog()) {
        if (iana == entry.iana) {
            return entry.posix;
        }
    }
    return "UTC0";
}

int timezoneOffsetSecondsAtUtc(const std::string &iana, int64_t utcSeconds) {
    if (iana == "America/New_York") {
        return inNewYorkDstUtc(utcSeconds) ? -4 * 3600 : -5 * 3600;
    }
    if (iana == "Europe/London") {
        return inEuropeDstUtc(iana, utcSeconds) ? 3600 : 0;
    }
    if (iana == "Europe/Berlin") {
        return inEuropeDstUtc(iana, utcSeconds) ? 2 * 3600 : 3600;
    }
    for (const TimezoneCatalogEntry &entry : timezoneCatalog()) {
        if (iana == entry.iana) {
            return entry.standardOffsetSeconds;
        }
    }
    return 0;
}
