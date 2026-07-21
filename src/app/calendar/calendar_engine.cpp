#include "calendar_engine.h"

#include <algorithm>

#include "app/calendar/calendar_cache_codec.h"

CalendarEngine::CalendarEngine(CacheStore &cache) : _cache(cache) {}

CalendarSnapshot CalendarEngine::merge(const std::vector<CalendarSourceSyncResult> &sources,
                                       uint32_t capacity) {
    CalendarSnapshot snapshot;
    std::vector<CalendarEvent> merged;

    for (const CalendarSourceSyncResult &source : sources) {
        SourceStatus status;
        status.sourceId = source.sourceId;
        status.state = source.state;
        status.itemCount = source.itemCount;

        if (source.state == SourceState::Ok) {
            const std::vector<uint8_t> encoded = encodeCalendarEvents(source.events);
            _cache.write(source.sourceId, encoded, source.updatedUtc);
            merged.insert(merged.end(), source.events.begin(), source.events.end());
            status.lastSuccessUtc = source.updatedUtc;
            status.lastAttemptUtc = source.updatedUtc;
            status.state = SourceState::Ok;
        } else {
            const CacheReadResult cached = _cache.read(source.sourceId);
            if (cached.ok) {
                const std::vector<CalendarEvent> events = decodeCalendarEvents(cached.payload);
                merged.insert(merged.end(), events.begin(), events.end());
                status.state = SourceState::Stale;
                status.itemCount = static_cast<uint32_t>(events.size());
                status.lastSuccessUtc = cached.updatedUtc;
            }
            status.lastAttemptUtc = source.updatedUtc;
        }
        snapshot.statuses.push_back(status);
    }

    std::sort(merged.begin(), merged.end(), [](const CalendarEvent &a, const CalendarEvent &b) {
        if (a.startUtc != b.startUtc) {
            return a.startUtc < b.startUtc;
        }
        if (a.sourceId != b.sourceId) {
            return a.sourceId < b.sourceId;
        }
        return a.uid < b.uid;
    });

    if (capacity > 0 && merged.size() > capacity) {
        merged.resize(capacity);
    }
    snapshot.events = std::move(merged);
    return snapshot;
}
