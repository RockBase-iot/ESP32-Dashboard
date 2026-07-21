#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/cache/cache_store.h"
#include "app/calendar/calendar_models.h"
#include "app/model/dashboard_models.h"

struct CalendarSourceSyncResult {
    std::string sourceId;
    SourceState state = SourceState::Stale;
    bool changed = false;
    std::vector<CalendarEvent> events;
    int64_t updatedUtc = 0;
    uint32_t itemCount = 0;
};

struct CalendarSnapshot {
    std::vector<CalendarEvent> events;
    std::vector<SourceStatus> statuses;
};

class CalendarEngine {
public:
    explicit CalendarEngine(CacheStore &cache);

    CalendarSnapshot merge(const std::vector<CalendarSourceSyncResult> &sources,
                           uint32_t capacity);

private:
    CacheStore &_cache;
};
