#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/calendar/calendar_models.h"
#include "ui/layouts/epd_400x300/render_overview.h"

CalendarPageSnapshot calendarPageSnapshotFromEvents(const std::vector<CalendarEvent> &events,
                                                    int64_t nowUtc,
                                                    const std::string &timezoneId,
                                                    bool twentyFourHour);
