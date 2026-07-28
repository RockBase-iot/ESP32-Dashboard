#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/calendar/calendar_models.h"
#include "ui/layouts/epd_400x300/render_overview.h"

enum class CalendarEmptyStateKind {
    SetupRequired,
    NoUsableData,
};

CalendarPageSnapshot calendarPageSnapshotFromEvents(const std::vector<CalendarEvent> &events,
                                                    int64_t nowUtc,
                                                    const std::string &timezoneId,
                                                    bool twentyFourHour);

CalendarPageSnapshot calendarEmptyStateSnapshot(CalendarEmptyStateKind kind,
                                                int64_t nowUtc,
                                                const std::string &timezoneId,
                                                bool twentyFourHour);
