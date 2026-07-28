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

int timezoneCatalogNthSunday(int year, int month, int ordinal) {
    const int firstDow = weekday(year, month, 1);
    const int firstSunday = 1 + ((7 - firstDow) % 7);
    return firstSunday + (ordinal - 1) * 7;
}

int timezoneCatalogLastSunday(int year, int month) {
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
    const int startDay = timezoneCatalogNthSunday(year, 3, 2);
    const int endDay = timezoneCatalogNthSunday(year, 11, 1);
    const int64_t start = timezoneCatalogDaysFromCivil(year, 3, static_cast<unsigned>(startDay)) * 86400LL + 7 * 3600LL;
    const int64_t end = timezoneCatalogDaysFromCivil(year, 11, static_cast<unsigned>(endDay)) * 86400LL + 6 * 3600LL;
    return utcSeconds >= start && utcSeconds < end;
}

bool inEuropeDstUtc(const std::string &iana, int64_t utcSeconds) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    timezoneCatalogCivilFromDays(utcSeconds / 86400LL, year, month, day);
    const int startDay = timezoneCatalogLastSunday(year, 3);
    const int endDay = timezoneCatalogLastSunday(year, 10);
    const int64_t start = timezoneCatalogDaysFromCivil(year, 3, static_cast<unsigned>(startDay)) * 86400LL + 1 * 3600LL;
    const int64_t end = timezoneCatalogDaysFromCivil(year, 10, static_cast<unsigned>(endDay)) * 86400LL + 1 * 3600LL;
    (void)iana;
    return utcSeconds >= start && utcSeconds < end;
}

bool inSydneyDstUtc(int64_t utcSeconds) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    timezoneCatalogCivilFromDays(utcSeconds / 86400LL, year, month, day);
    const int startDay = timezoneCatalogNthSunday(year, 10, 1);
    const int endDay = timezoneCatalogNthSunday(year, 4, 1);
    const int64_t start = timezoneCatalogDaysFromCivil(year, 10, static_cast<unsigned>(startDay)) * 86400LL - 10 * 3600LL + 2 * 3600LL;
    const int64_t end = timezoneCatalogDaysFromCivil(year, 4, static_cast<unsigned>(endDay)) * 86400LL - 11 * 3600LL + 3 * 3600LL;
    return utcSeconds >= start || utcSeconds < end;
}

const TimezoneCatalogEntry *findTimezoneEntry(const std::string &iana) {
    for (const TimezoneCatalogEntry &entry : timezoneCatalog()) {
        if (iana == entry.iana) {
            return &entry;
        }
    }
    return nullptr;
}
}  // namespace

const std::vector<TimezoneCatalogEntry> &timezoneCatalog() {
    static const std::vector<TimezoneCatalogEntry> kCatalog = {
        {"Etc/UTC", "UTC0", 0, 0, false},
        {"UTC", "UTC0", 0, 0, false},
        {"GMT", "UTC0", 0, 0, false},
        {"Asia/Shanghai", "CST-8", 8 * 3600, 8 * 3600, false},
        {"Asia/Hong_Kong", "HKT-8", 8 * 3600, 8 * 3600, false},
        {"Asia/Singapore", "SGT-8", 8 * 3600, 8 * 3600, false},
        {"Asia/Taipei", "CST-8", 8 * 3600, 8 * 3600, false},
        {"Asia/Kuala_Lumpur", "MYT-8", 8 * 3600, 8 * 3600, false},
        {"Asia/Manila", "PHT-8", 8 * 3600, 8 * 3600, false},
        {"Asia/Tokyo", "JST-9", 9 * 3600, 9 * 3600, false},
        {"Asia/Seoul", "KST-9", 9 * 3600, 9 * 3600, false},
        {"Asia/Bangkok", "ICT-7", 7 * 3600, 7 * 3600, false},
        {"Asia/Jakarta", "WIB-7", 7 * 3600, 7 * 3600, false},
        {"Asia/Ho_Chi_Minh", "ICT-7", 7 * 3600, 7 * 3600, false},
        {"Asia/Kolkata", "IST-5:30", 19800, 19800, false},
        {"Asia/Dubai", "GST-4", 4 * 3600, 4 * 3600, false},
        {"Asia/Riyadh", "AST-3", 3 * 3600, 3 * 3600, false},
        {"Asia/Istanbul", "TRT-3", 3 * 3600, 3 * 3600, false},
        {"Asia/Jerusalem", "IST-2IDT,M3.4.4/26,M10.5.0", 2 * 3600, 3 * 3600, true},
        {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0/2", 0, 3600, true},
        {"Europe/Dublin", "IST-1GMT0,M10.5.0,M3.5.0/1", 0, 3600, true},
        {"Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0/2", 0, 3600, true},
        {"Europe/Berlin", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Paris", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Budapest", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Warsaw", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Chisinau", "EET-2EEST,M3.5.0/3,M10.5.0/4", 2 * 3600, 3 * 3600, true},
        {"Europe/Rome", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Madrid", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Amsterdam", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Zurich", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Stockholm", "CET-1CEST,M3.5.0/2,M10.5.0/3", 3600, 2 * 3600, true},
        {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4", 2 * 3600, 3 * 3600, true},
        {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4", 2 * 3600, 3 * 3600, true},
        {"Europe/Moscow", "MSK-3", 3 * 3600, 3 * 3600, false},
        {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2", -5 * 3600, -4 * 3600, true},
        {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2", -6 * 3600, -5 * 3600, true},
        {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2", -7 * 3600, -6 * 3600, true},
        {"America/Phoenix", "MST7", -7 * 3600, -7 * 3600, false},
        {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2", -8 * 3600, -7 * 3600, true},
        {"America/Anchorage", "AKST9AKDT,M3.2.0/2,M11.1.0/2", -9 * 3600, -8 * 3600, true},
        {"Pacific/Honolulu", "HST10", -10 * 3600, -10 * 3600, false},
        {"America/Toronto", "EST5EDT,M3.2.0/2,M11.1.0/2", -5 * 3600, -4 * 3600, true},
        {"America/Vancouver", "PST8PDT,M3.2.0/2,M11.1.0/2", -8 * 3600, -7 * 3600, true},
        {"America/Regina", "CST6", -6 * 3600, -6 * 3600, false},
        {"America/Mexico_City", "CST6CDT,M4.1.0/2,M10.5.0/2", -6 * 3600, -5 * 3600, false},
        {"America/Puerto_Rico", "AST4", -4 * 3600, -4 * 3600, false},
        {"America/Bogota", "COT5", -5 * 3600, -5 * 3600, false},
        {"America/Lima", "PET5", -5 * 3600, -5 * 3600, false},
        {"America/Santiago", "CLT4", -4 * 3600, -4 * 3600, false},
        {"America/Argentina/Buenos_Aires", "ART3", -3 * 3600, -3 * 3600, false},
        {"America/Sao_Paulo", "BRT3", -3 * 3600, -3 * 3600, false},
        {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3", 10 * 3600, 11 * 3600, true},
        {"Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3", 10 * 3600, 11 * 3600, true},
        {"Australia/Brisbane", "AEST-10", 10 * 3600, 10 * 3600, false},
        {"Australia/Perth", "AWST-8", 8 * 3600, 8 * 3600, false},
        {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3", 12 * 3600, 13 * 3600, true},
        {"Africa/Johannesburg", "SAST-2", 2 * 3600, 2 * 3600, false},
        {"Africa/Cairo", "EET-2", 2 * 3600, 2 * 3600, false},
        {"Africa/Nairobi", "EAT-3", 3 * 3600, 3 * 3600, false},
    };
    return kCatalog;
}

std::string canonicalTimezoneId(const std::string &timezoneId) {
    static const std::pair<const char *, const char *> kAliases[] = {
        {"Dateline Standard Time", "Etc/UTC"},
        {"UTC", "UTC"},
        {"GMT Standard Time", "Europe/London"},
        {"Greenwich Standard Time", "GMT"},
        {"W. Europe Standard Time", "Europe/Berlin"},
        {"Central Europe Standard Time", "Europe/Budapest"},
        {"Romance Standard Time", "Europe/Paris"},
        {"Central European Standard Time", "Europe/Warsaw"},
        {"E. Europe Standard Time", "Europe/Chisinau"},
        {"China Standard Time", "Asia/Shanghai"},
        {"Taipei Standard Time", "Asia/Taipei"},
        {"Singapore Standard Time", "Asia/Singapore"},
        {"Tokyo Standard Time", "Asia/Tokyo"},
        {"Korea Standard Time", "Asia/Seoul"},
        {"SE Asia Standard Time", "Asia/Bangkok"},
        {"India Standard Time", "Asia/Kolkata"},
        {"Arabian Standard Time", "Asia/Dubai"},
        {"Eastern Standard Time", "America/New_York"},
        {"Central Standard Time", "America/Chicago"},
        {"Mountain Standard Time", "America/Denver"},
        {"US Mountain Standard Time", "America/Phoenix"},
        {"Pacific Standard Time", "America/Los_Angeles"},
        {"Canada Central Standard Time", "America/Regina"},
        {"SA Eastern Standard Time", "America/Sao_Paulo"},
        {"AUS Eastern Standard Time", "Australia/Sydney"},
        {"E. Australia Standard Time", "Australia/Brisbane"},
        {"W. Australia Standard Time", "Australia/Perth"},
        {"New Zealand Standard Time", "Pacific/Auckland"},
        {"South Africa Standard Time", "Africa/Johannesburg"},
    };
    for (const auto &alias : kAliases) {
        if (timezoneId == alias.first) {
            return alias.second;
        }
    }
    return timezoneId;
}

std::string timezonePosixRule(const std::string &iana) {
    const std::string canonical = canonicalTimezoneId(iana);
    for (const TimezoneCatalogEntry &entry : timezoneCatalog()) {
        if (canonical == entry.iana) {
            return entry.posix;
        }
    }
    return "UTC0";
}

int timezoneOffsetSecondsAtUtc(const std::string &iana, int64_t utcSeconds) {
    const std::string canonical = canonicalTimezoneId(iana);
    if (canonical == "America/New_York" || canonical == "America/Toronto") {
        return inNewYorkDstUtc(utcSeconds) ? -4 * 3600 : -5 * 3600;
    }
    if (canonical == "America/Chicago") {
        return inNewYorkDstUtc(utcSeconds) ? -5 * 3600 : -6 * 3600;
    }
    if (canonical == "America/Denver") {
        return inNewYorkDstUtc(utcSeconds) ? -6 * 3600 : -7 * 3600;
    }
    if (canonical == "America/Los_Angeles" || canonical == "America/Vancouver") {
        return inNewYorkDstUtc(utcSeconds) ? -7 * 3600 : -8 * 3600;
    }
    if (canonical == "America/Anchorage") {
        return inNewYorkDstUtc(utcSeconds) ? -8 * 3600 : -9 * 3600;
    }
    if (canonical == "Europe/London" || canonical == "Europe/Dublin" ||
        canonical == "Europe/Lisbon") {
        return inEuropeDstUtc(canonical, utcSeconds) ? 3600 : 0;
    }
    if (canonical.rfind("Europe/", 0) == 0) {
        const TimezoneCatalogEntry *entry = findTimezoneEntry(canonical);
        if (entry && !entry->observesDst) {
            return entry->standardOffsetSeconds;
        }
        const int standard = entry ? entry->standardOffsetSeconds : 3600;
        return inEuropeDstUtc(canonical, utcSeconds) ? standard + 3600 : standard;
    }
    if (canonical == "Australia/Sydney" || canonical == "Australia/Melbourne") {
        return inSydneyDstUtc(utcSeconds) ? 11 * 3600 : 10 * 3600;
    }
    const TimezoneCatalogEntry *entry = findTimezoneEntry(canonical);
    if (entry) {
        return entry->standardOffsetSeconds;
    }
    return 0;
}
