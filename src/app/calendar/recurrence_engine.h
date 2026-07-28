#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/calendar/calendar_models.h"
#include "app/calendar/recurrence_rule.h"
#include "app/model/dashboard_models.h"

struct RecurrenceWindow {
    int64_t startUtc = 0;
    int64_t endUtc = 0;
    int64_t seedUtc = 0;
    uint32_t capacity = 0;
};

class RecurrenceEngine {
public:
    std::vector<CalendarEvent> expand(const std::vector<CalendarEvent> &events,
                                      const RecurrenceWindow &window,
                                      const std::string &defaultTimezoneId,
                                      SourceState &state) const;
};
