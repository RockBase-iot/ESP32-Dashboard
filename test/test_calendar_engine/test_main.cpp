#include <unity.h>

#include "app/cache/cache_store.h"
#include "app/cache/cache_store.cpp"
#include "app/calendar/calendar_models.h"
#include "app/calendar/calendar_models.cpp"
#include "app/calendar/calendar_cache_codec.h"
#include "app/calendar/calendar_cache_codec.cpp"
#include "app/calendar/calendar_engine.h"
#include "app/calendar/calendar_engine.cpp"

void test_calendar_cache_round_trips_events() {
    CalendarEvent event;
    event.sourceId = "calendar00";
    event.uid = "uid-1";
    event.summary = "Design review";
    event.startUtc = 1784552400LL;
    event.endUtc = 1784556000LL;

    const auto bytes = encodeCalendarEvents({event});
    const auto decoded = decodeCalendarEvents(bytes);

    TEST_ASSERT_EQUAL_UINT32(1, decoded.size());
    TEST_ASSERT_EQUAL_STRING("Design review", decoded[0].summary.c_str());
}

void test_engine_preserves_previous_source_cache_when_new_source_fails() {
    MemoryCacheBackend backend;
    CacheStore cache(backend, "/cache");
    CalendarEngine engine(cache);
    CalendarEvent previous;
    previous.sourceId = "calendar00";
    previous.uid = "old";
    previous.summary = "Cached";
    previous.startUtc = 1784552400LL;
    previous.endUtc = 1784556000LL;
    cache.write("calendar00", encodeCalendarEvents({previous}), 1784500000LL);

    CalendarSourceSyncResult failed;
    failed.sourceId = "calendar00";
    failed.state = SourceState::Tls;
    failed.updatedUtc = 1784600000LL;

    const CalendarSnapshot snapshot = engine.merge({failed}, 250);

    TEST_ASSERT_EQUAL_UINT32(1, snapshot.events.size());
    TEST_ASSERT_EQUAL_STRING("Cached", snapshot.events[0].summary.c_str());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Stale),
                            static_cast<uint8_t>(snapshot.statuses[0].state));
    TEST_ASSERT_EQUAL_INT64(1784500000LL, snapshot.statuses[0].lastSuccessUtc);
    TEST_ASSERT_EQUAL_INT64(1784600000LL, snapshot.statuses[0].lastAttemptUtc);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_calendar_cache_round_trips_events);
    RUN_TEST(test_engine_preserves_previous_source_cache_when_new_source_fails);
    UNITY_END();
}

void loop() {}
