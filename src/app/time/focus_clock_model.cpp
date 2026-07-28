#include "focus_clock_model.h"

#include <algorithm>
#include <cstdio>

#include "app/time/timezone_catalog.h"

namespace {
constexpr const char *kFocusWeekdayNames[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
};
constexpr const char *kFocusMonthNames[] = {
    "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
};

int64_t daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

void civilFromDays(int64_t z, int &year, unsigned &month, unsigned &day) {
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

void localParts(int64_t utc, const std::string &timezoneId,
                int &year, unsigned &month, unsigned &day, int &hour, int &minute) {
    const int64_t localSeconds = utc + timezoneOffsetSecondsAtUtc(timezoneId, utc);
    civilFromDays(localSeconds / 86400LL, year, month, day);
    int rem = static_cast<int>(localSeconds % 86400LL);
    if (rem < 0) {
        rem += 86400;
    }
    hour = rem / 3600;
    minute = (rem % 3600) / 60;
}

std::string focusFormatClock(int hour, int minute, bool twentyFourHour) {
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

std::string formatLocalTime(int64_t utc, const std::string &timezoneId, bool twentyFourHour) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    int hour = 0;
    int minute = 0;
    localParts(utc, timezoneId, year, month, day, hour, minute);
    return focusFormatClock(hour, minute, twentyFourHour);
}

std::string formatChromeLabel(int64_t utc, const std::string &timezoneId, bool twentyFourHour) {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    int hour = 0;
    int minute = 0;
    localParts(utc, timezoneId, year, month, day, hour, minute);

    int weekday = static_cast<int>((daysFromCivil(year, month, day) + 4) % 7);
    if (weekday < 0) {
        weekday += 7;
    }

    char buffer[40];
    const std::string clockText = focusFormatClock(hour, minute, twentyFourHour);
    std::snprintf(buffer, sizeof(buffer), "%s %s %s %02u, %04d",
                  kFocusWeekdayNames[weekday], clockText.c_str(),
                  month <= 12 ? kFocusMonthNames[month] : "JAN",
                  day, year);
    return std::string(buffer);
}

std::string formatDurationMinutes(int64_t seconds) {
    if (seconds < 0) {
        seconds = 0;
    }
    const int minutes = static_cast<int>((seconds + 59) / 60);
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%d MIN", minutes);
    return std::string(buffer);
}

std::string formatMinutes(uint16_t minutes, const char *suffix) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%u MIN %s",
                  static_cast<unsigned>(minutes), suffix);
    return std::string(buffer);
}

std::string formatCycle(uint8_t cycle, uint8_t total) {
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "CYCLE %u OF %u",
                  static_cast<unsigned>(cycle), static_cast<unsigned>(total));
    return std::string(buffer);
}
}  // namespace

FocusClockConfig normalizeFocusClockConfig(FocusClockConfig config) {
    config.focusMinutes = std::max<uint16_t>(1, std::min<uint16_t>(180, config.focusMinutes));
    config.breakMinutes = std::max<uint16_t>(1, std::min<uint16_t>(60, config.breakMinutes));
    config.sessionCount = std::max<uint8_t>(1, std::min<uint8_t>(12, config.sessionCount));
    if (config.label.empty()) {
        config.label = "Focus";
    }
    return config;
}

FocusClockConfig defaultFocusClockConfig() {
    return normalizeFocusClockConfig(FocusClockConfig());
}

FocusClockRuntimeState startFocusClockSession(const FocusClockConfig &, int64_t nowUtc) {
    FocusClockRuntimeState state;
    state.active = true;
    state.startedUtc = nowUtc;
    return state;
}

FocusClockRuntimeState stopFocusClockSession() {
    return FocusClockRuntimeState();
}

bool focusClockSessionIsActive(const FocusClockConfig &config,
                               const FocusClockRuntimeState &state,
                               int64_t nowUtc) {
    const FocusClockConfig clean = normalizeFocusClockConfig(config);
    const int64_t cycleSec = static_cast<int64_t>(clean.focusMinutes + clean.breakMinutes) * 60LL;
    const int64_t totalSec = cycleSec * clean.sessionCount;
    return state.active && state.startedUtc > 0 && nowUtc >= state.startedUtc &&
           nowUtc - state.startedUtc < totalSec;
}

FocusClockPageSnapshot focusClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount,
                                                const FocusClockConfig &config,
                                                const FocusClockRuntimeState &state,
                                                bool twentyFourHour) {
    const FocusClockConfig clean = normalizeFocusClockConfig(config);
    const int64_t focusSec = static_cast<int64_t>(clean.focusMinutes) * 60LL;
    const int64_t breakSec = static_cast<int64_t>(clean.breakMinutes) * 60LL;
    const int64_t cycleSec = focusSec + breakSec;

    FocusClockPageSnapshot snapshot;
    snapshot.title = "FOCUS CLOCK";
    snapshot.subtitle = formatChromeLabel(nowUtc, timezoneId, twentyFourHour);
    snapshot.pageIndicator = std::to_string(pageNumber) + "/" + std::to_string(pageCount);
    snapshot.focusDurationText = formatMinutes(clean.focusMinutes, "FOCUS");
    snapshot.breakDurationText = formatMinutes(clean.breakMinutes, "BREAK");
    snapshot.controlText = "USER HOLD 2S START / STOP";
    snapshot.nextBreakText = "--:--";
    snapshot.endTimeText = "--:--";

    if (!focusClockSessionIsActive(clean, state, nowUtc)) {
        snapshot.countdownText = formatDurationMinutes(focusSec);
        snapshot.statusText = state.active ? "COMPLETE" : "READY";
        snapshot.cycleText = formatCycle(state.active ? clean.sessionCount : 1, clean.sessionCount);
        if (state.active) {
            snapshot.countdownText = "0 MIN";
        }
        return snapshot;
    }

    const int64_t elapsed = nowUtc - state.startedUtc;
    const uint8_t cycle = static_cast<uint8_t>(elapsed / cycleSec) + 1;
    const int64_t inCycle = elapsed % cycleSec;
    const int64_t cycleStart = state.startedUtc + static_cast<int64_t>(cycle - 1) * cycleSec;
    const int64_t nextBreakUtc = cycleStart + focusSec;
    const int64_t cycleEndUtc = cycleStart + cycleSec;

    snapshot.active = true;
    snapshot.cycleText = formatCycle(cycle, clean.sessionCount);
    snapshot.nextBreakText = formatLocalTime(nextBreakUtc, timezoneId, twentyFourHour);
    snapshot.endTimeText = formatLocalTime(cycleEndUtc, timezoneId, twentyFourHour);
    if (inCycle < focusSec) {
        snapshot.statusText = "IN FOCUS";
        snapshot.countdownText = formatDurationMinutes(focusSec - inCycle);
    } else {
        snapshot.statusText = "BREAK";
        snapshot.countdownText = formatDurationMinutes(cycleSec - inCycle);
        snapshot.inBreak = true;
    }
    return snapshot;
}
