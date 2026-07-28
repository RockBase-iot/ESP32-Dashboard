#include "world_clock_model.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

#include "app/time/timezone_catalog.h"

namespace {
constexpr const char *kWeekdayNames[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
};
constexpr const char *kMonthNames[] = {
    "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
};

int64_t worldClockDaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

void worldClockCivilFromDays(int64_t z, int &year, unsigned &month, unsigned &day) {
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

std::string formatClock(int hour, int minute, bool twentyFourHour) {
    char buffer[16];
    if (twentyFourHour) {
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
    } else {
        const char *suffix = hour >= 12 ? "PM" : "AM";
        int displayHour = hour % 12;
        if (displayHour == 0) {
            displayHour = 12;
        }
        std::snprintf(buffer, sizeof(buffer), "%d:%02d %s", displayHour, minute, suffix);
    }
    return std::string(buffer);
}

std::string trimAscii(std::string value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

bool looksLikeTimezoneId(const std::string &value) {
    return value.find('/') != std::string::npos ||
           value.rfind("Etc/", 0) == 0 ||
           value == "UTC" ||
           value == "GMT";
}

void appendZoneIfValid(WorldClockConfig &config, const std::string &zoneId,
                       const std::string &label) {
    if (config.zones.size() >= 4 || zoneId.empty() || label.empty()) {
        return;
    }
    config.zones.push_back({label, zoneId});
}

void fillDefaultZones(WorldClockConfig &config) {
    config.zones.clear();
    config.zones.push_back({"Shanghai", "Asia/Shanghai"});
    config.zones.push_back({"New York", "America/New_York"});
    config.zones.push_back({"London", "Europe/London"});
    config.zones.push_back({"Tokyo", "Asia/Tokyo"});
}

std::string fallbackLabelForZone(std::string zoneId) {
    const size_t slash = zoneId.rfind('/');
    if (slash != std::string::npos && slash + 1 < zoneId.size()) {
        zoneId = zoneId.substr(slash + 1);
    }
    std::replace(zoneId.begin(), zoneId.end(), '_', ' ');
    return zoneId;
}

std::string formatUtcOffset(int offsetSeconds) {
    const char sign = offsetSeconds >= 0 ? '+' : '-';
    int minutes = std::abs(offsetSeconds) / 60;
    const int hours = minutes / 60;
    minutes %= 60;
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "UTC%c%02d", sign, hours);
    if (minutes != 0) {
        std::snprintf(buffer, sizeof(buffer), "UTC%c%02d:%02d", sign, hours, minutes);
    }
    return std::string(buffer);
}

std::string timePhaseForHour(int hour) {
    if (hour >= 6 && hour < 9) {
        return "MORNING";
    }
    if (hour >= 9 && hour < 18) {
        return "WORKING";
    }
    if (hour >= 18 && hour < 23) {
        return "EVENING";
    }
    return "NIGHT";
}

void civilTimeForZone(int64_t nowUtc, const std::string &timezoneId,
                      int &year, unsigned &month, unsigned &day,
                      int &hour, int &minute) {
    const int offset = timezoneOffsetSecondsAtUtc(timezoneId, nowUtc);
    const int64_t localSeconds = nowUtc + offset;
    worldClockCivilFromDays(localSeconds / 86400LL, year, month, day);
    int rem = static_cast<int>(localSeconds % 86400LL);
    if (rem < 0) {
        rem += 86400;
    }
    hour = rem / 3600;
    minute = (rem % 3600) / 60;
}
}  // namespace

WorldClockConfig sampleWorldClockConfig() {
    WorldClockConfig config;
    config.twentyFourHour = true;
    config.zones.push_back({"New York", "America/New_York"});
    config.zones.push_back({"London", "Europe/London"});
    config.zones.push_back({"Berlin", "Europe/Berlin"});
    config.zones.push_back({"Shanghai", "Asia/Shanghai"});
    config.focusLabel = "Focus Clock";
    config.focusText = "Next sync at 14:30";
    return config;
}

WorldClockConfig worldClockConfigFromZonesText(const std::string &zonesText,
                                               const std::string &focusLabel,
                                               bool twentyFourHour) {
    WorldClockConfig config;
    config.twentyFourHour = twentyFourHour;
    config.focusLabel = focusLabel.empty() ? "FOCUS CLOCK" : focusLabel;
    config.focusText = "Next sync at 14:30";

    size_t start = 0;
    while (start <= zonesText.size() && config.zones.size() < 4) {
        size_t end = zonesText.find_first_of(",\n\r", start);
        if (end == std::string::npos) {
            end = zonesText.size();
        }
        const std::string token = trimAscii(zonesText.substr(start, end - start));
        if (!token.empty()) {
            const size_t sep = token.find('|');
            if (sep == std::string::npos) {
                const std::string zone = trimAscii(token);
                appendZoneIfValid(config, zone, fallbackLabelForZone(zone));
            } else {
                const std::string first = trimAscii(token.substr(0, sep));
                const std::string second = trimAscii(token.substr(sep + 1));
                if (looksLikeTimezoneId(first)) {
                    appendZoneIfValid(config, first, second.empty() ? fallbackLabelForZone(first) : second);
                } else {
                    appendZoneIfValid(config, second, first);
                }
            }
        }
        if (end == zonesText.size()) {
            break;
        }
        start = end + 1;
    }

    if (config.zones.empty()) {
        fillDefaultZones(config);
    }
    return config;
}

WorldClockModel buildWorldClock(const WorldClockConfig &config, int64_t nowUtc) {
    WorldClockModel model;
    model.focusLabel = config.focusLabel;
    model.focusText = config.focusText;

    int utcYear = 1970;
    unsigned utcMonth = 1;
    unsigned utcDay = 1;
    worldClockCivilFromDays(nowUtc / 86400LL, utcYear, utcMonth, utcDay);
    const int64_t utcDayCount = worldClockDaysFromCivil(utcYear, utcMonth, utcDay);

    for (const WorldClockZone &zone : config.zones) {
        const int offset = timezoneOffsetSecondsAtUtc(zone.timezoneId, nowUtc);
        const int64_t localUtc = nowUtc + offset;
        int year = 1970;
        unsigned month = 1;
        unsigned day = 1;
        worldClockCivilFromDays(localUtc / 86400LL, year, month, day);
        int rem = static_cast<int>(localUtc % 86400LL);
        if (rem < 0) {
            rem += 86400;
        }
        WorldClockSlot slot;
        slot.label = zone.label;
        slot.timezoneId = zone.timezoneId;
        slot.timeText = formatClock(rem / 3600, (rem % 3600) / 60, config.twentyFourHour);
        slot.statusText = formatUtcOffset(offset) + " - " + timePhaseForHour(rem / 3600);
        char dateBuffer[32];
        std::snprintf(dateBuffer, sizeof(dateBuffer), "%04d-%02u-%02u", year, month, day);
        slot.dateText = dateBuffer;
        slot.dayDelta = static_cast<int>(worldClockDaysFromCivil(year, month, day) - utcDayCount);
        model.clocks.push_back(slot);
    }
    return model;
}

std::string formatWorldClockDateLabel(int64_t nowUtc, const std::string &timezoneId) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    int hour = 0;
    int minute = 0;
    civilTimeForZone(nowUtc, timezoneId, year, month, day, hour, minute);

    const int64_t days = worldClockDaysFromCivil(year, month, day);
    int weekday = static_cast<int>((days + 4) % 7);
    if (weekday < 0) {
        weekday += 7;
    }

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%s %s %02u, %04d",
                  kWeekdayNames[weekday],
                  month <= 12 ? kMonthNames[month] : "JAN",
                  day, year);
    return std::string(buffer);
}

std::string formatWorldClockChromeTimeLabel(int64_t nowUtc, const std::string &timezoneId,
                                            bool twentyFourHour) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    int hour = 0;
    int minute = 0;
    civilTimeForZone(nowUtc, timezoneId, year, month, day, hour, minute);

    const int64_t days = worldClockDaysFromCivil(year, month, day);
    int weekday = static_cast<int>((days + 4) % 7);
    if (weekday < 0) {
        weekday += 7;
    }

    char buffer[40];
    const std::string clockText = formatClock(hour, minute, twentyFourHour);
    std::snprintf(buffer, sizeof(buffer), "%s %s %s %02u, %04d",
                  kWeekdayNames[weekday], clockText.c_str(),
                  month <= 12 ? kMonthNames[month] : "JAN",
                  day, year);
    return std::string(buffer);
}
