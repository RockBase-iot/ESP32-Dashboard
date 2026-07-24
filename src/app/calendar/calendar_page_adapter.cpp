#include "calendar_page_adapter.h"

#include <algorithm>
#include <cstdio>
#include <ctime>

#include "app/calendar/timezone_resolver.h"

namespace {
struct LocalParts {
    int year = 1970;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    int yday = 0;
    int wday = 0;
    int64_t localDay = 0;
};

int64_t floorDiv(int64_t value, int64_t divisor) {
    int64_t quotient = value / divisor;
    const int64_t remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) {
        --quotient;
    }
    return quotient;
}

LocalParts localPartsFromEpoch(int64_t localEpoch) {
    const time_t localTime = static_cast<time_t>(localEpoch);
    struct tm tmValue {};
#if defined(_WIN32)
    gmtime_s(&tmValue, &localTime);
#else
    gmtime_r(&localTime, &tmValue);
#endif
    LocalParts parts;
    parts.year = tmValue.tm_year + 1900;
    parts.month = tmValue.tm_mon + 1;
    parts.day = tmValue.tm_mday;
    parts.hour = tmValue.tm_hour;
    parts.minute = tmValue.tm_min;
    parts.yday = tmValue.tm_yday;
    parts.wday = tmValue.tm_wday;
    parts.localDay = floorDiv(localEpoch, 86400LL);
    return parts;
}

LocalParts localPartsFor(int64_t utc, const std::string &timezoneId) {
    TimezoneResolver resolver;
    return localPartsFromEpoch(resolver.fromUtc(timezoneId, utc));
}

int dateKey(const LocalParts &parts) {
    return parts.year * 10000 + parts.month * 100 + parts.day;
}

std::string shortTitle(const CalendarEvent &event) {
    if (!event.summary.empty()) {
        return event.summary;
    }
    return "Untitled event";
}

std::string detailText(const CalendarEvent &event) {
    if (!event.location.empty()) {
        return event.location;
    }
    if (!event.sourceId.empty()) {
        return event.sourceId;
    }
    return "Calendar";
}

std::string timeTextFor(const CalendarEvent &event, const LocalParts &parts, bool twentyFourHour) {
    if (event.allDay) {
        return "ALL DAY";
    }
    char buffer[16];
    if (twentyFourHour) {
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", parts.hour, parts.minute);
    } else {
        const int hour12 = parts.hour % 12 == 0 ? 12 : parts.hour % 12;
        std::snprintf(buffer, sizeof(buffer), "%d:%02d%s", hour12, parts.minute,
                      parts.hour < 12 ? "A" : "P");
    }
    return std::string(buffer);
}

std::string agendaTextFor(const CalendarEvent &event, const LocalParts &parts,
                          bool twentyFourHour) {
    return timeTextFor(event, parts, twentyFourHour) + "  " + shortTitle(event);
}

const char *monthName(int month) {
    static constexpr const char *kMonths[] = {
        "", "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
        "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER",
    };
    return month >= 1 && month <= 12 ? kMonths[month] : "JANUARY";
}

const char *monthShortName(int month) {
    static constexpr const char *kMonths[] = {
        "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
    };
    return month >= 1 && month <= 12 ? kMonths[month] : "JAN";
}

const char *weekdayName(int wday) {
    static constexpr const char *kDays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    return wday >= 0 && wday <= 6 ? kDays[wday] : "MON";
}

std::string monthTitleFor(const LocalParts &today) {
    return std::string(monthName(today.month)) + " " + std::to_string(today.year);
}

std::string weekRangeLabelFor(int64_t weekStartDay) {
    const LocalParts start = localPartsFromEpoch(weekStartDay * 86400LL + 12LL * 3600LL);
    const LocalParts end = localPartsFromEpoch((weekStartDay + 6) * 86400LL + 12LL * 3600LL);
    char buffer[40];
    if (start.month == end.month && start.year == end.year) {
        std::snprintf(buffer, sizeof(buffer), "%s %02d-%02d %04d",
                      monthShortName(start.month), start.day, end.day, start.year);
    } else if (start.year == end.year) {
        std::snprintf(buffer, sizeof(buffer), "%s %02d-%s %02d %04d",
                      monthShortName(start.month), start.day,
                      monthShortName(end.month), end.day, start.year);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%s %02d %04d-%s %02d %04d",
                      monthShortName(start.month), start.day, start.year,
                      monthShortName(end.month), end.day, end.year);
    }
    return std::string(buffer);
}

std::vector<CalendarDayCell> weekCellsFor(const LocalParts &today, int64_t weekStartDay) {
    std::vector<CalendarDayCell> cells;
    cells.reserve(7);
    for (int i = 0; i < 7; ++i) {
        const LocalParts parts = localPartsFromEpoch((weekStartDay + i) * 86400LL + 12LL * 3600LL);
        CalendarDayCell cell;
        cell.text = std::to_string(parts.day);
        cell.detail = weekdayName(parts.wday);
        cell.today = parts.localDay == today.localDay;
        cell.accent = cell.today;
        cells.push_back(cell);
    }
    return cells;
}

bool eventVisible(const CalendarEvent &event) {
    return event.status != CalendarEventStatus::Cancelled && event.startUtc > 0;
}
}  // namespace

CalendarPageSnapshot calendarPageSnapshotFromEvents(const std::vector<CalendarEvent> &events,
                                                    int64_t nowUtc,
                                                    const std::string &timezoneId,
                                                    bool twentyFourHour) {
    CalendarPageSnapshot snapshot;
    snapshot.title = "TODAY OVERVIEW";
    snapshot.subtitle = "Calendar";
    snapshot.notes = {
        "Calendar source synced",
        "BOOT/USER switches pages",
        "Deep sleep keeps page",
    };
    snapshot.milestones = {
        "Live iCal enabled",
        "Local cache fallback",
        "No OAuth required",
    };

    std::vector<CalendarEvent> sorted;
    sorted.reserve(events.size());
    for (const CalendarEvent &event : events) {
        if (eventVisible(event)) {
            sorted.push_back(event);
        }
    }
    std::sort(sorted.begin(), sorted.end(), [](const CalendarEvent &a, const CalendarEvent &b) {
        if (a.startUtc != b.startUtc) {
            return a.startUtc < b.startUtc;
        }
        return a.uid < b.uid;
    });

    const LocalParts today = localPartsFor(nowUtc, timezoneId);
    const int todayKey = dateKey(today);
    const int daysSinceMonday = (today.wday + 6) % 7;
    const int64_t weekStartDay = today.localDay - daysSinceMonday;
    snapshot.dateTitle = monthTitleFor(today);
    snapshot.weekRangeLabel = weekRangeLabelFor(weekStartDay);
    snapshot.weekCells = weekCellsFor(today, weekStartDay);
    for (const CalendarEvent &event : sorted) {
        const LocalParts start = localPartsFor(event.startUtc, timezoneId);
        const std::string timeText = timeTextFor(event, start, twentyFourHour);
        const int dayIndex = static_cast<int>(start.localDay - weekStartDay);
        const bool activeNow = !event.allDay && event.startUtc <= nowUtc &&
                               (event.endUtc == 0 || event.endUtc > nowUtc);
        const bool todayEvent = dateKey(start) == todayKey;
        const CalendarEventLine line{timeText, shortTitle(event), detailText(event),
                                     activeNow || todayEvent, dayIndex};

        if (event.startUtc >= nowUtc && snapshot.overviewItems.size() < 4) {
            snapshot.overviewItems.push_back(line);
        }
        if (dayIndex >= 0 && dayIndex < 7 && snapshot.timelineItems.size() < 14) {
            snapshot.timelineItems.push_back(line);
        }
        if (todayEvent && snapshot.agendaItems.size() < 6) {
            snapshot.agendaItems.push_back(agendaTextFor(event, start, twentyFourHour));
        }
    }

    if (snapshot.overviewItems.empty()) {
        snapshot.overviewItems.push_back({"--", "No upcoming calendar events", "iCal source", false});
    }
    if (snapshot.timelineItems.empty()) {
        snapshot.timelineItems.push_back({"--", "No events this week", "Calendar", false, 0});
    }
    if (snapshot.agendaItems.empty()) {
        snapshot.agendaItems.push_back("No calendar events today");
    }
    return snapshot;
}
