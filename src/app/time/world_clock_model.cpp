#include "world_clock_model.h"

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
        char dateBuffer[32];
        std::snprintf(dateBuffer, sizeof(dateBuffer), "%04d-%02u-%02u", year, month, day);
        slot.dateText = dateBuffer;
        slot.dayDelta = static_cast<int>(worldClockDaysFromCivil(year, month, day) - utcDayCount);
        model.clocks.push_back(slot);
    }
    return model;
}

std::string formatWorldClockDateLabel(int64_t nowUtc, const std::string &timezoneId) {
    const int offset = timezoneOffsetSecondsAtUtc(timezoneId, nowUtc);
    const int64_t localSeconds = nowUtc + offset;
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    worldClockCivilFromDays(localSeconds / 86400LL, year, month, day);

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
