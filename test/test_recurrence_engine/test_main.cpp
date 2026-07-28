#include <unity.h>

#include "app/calendar/calendar_models.h"
#include "app/calendar/calendar_models.cpp"
#include "app/calendar/recurrence_rule.h"
#include "app/calendar/recurrence_rule.cpp"
#include "app/calendar/timezone_resolver.h"
#include "app/time/timezone_catalog.cpp"
#include "app/calendar/timezone_resolver.cpp"
#include "app/calendar/recurrence_engine.h"
#include "app/calendar/recurrence_engine.cpp"

namespace {
CalendarEvent baseWeekly() {
    CalendarEvent event;
    event.sourceId = "calendar00";
    event.uid = "weekly@example.com";
    event.summary = "Team Sync";
    event.startUtc = 1784552400LL;
    event.endUtc = 1784556000LL;
    event.rrule = "FREQ=WEEKLY;COUNT=4;BYDAY=MO";
    return event;
}
}  // namespace

void test_weekly_rule_expands_count_in_window() {
    RecurrenceEngine engine;
    RecurrenceWindow window{1783900800LL, 1786924800LL, 1784552400LL, 250};
    SourceState state = SourceState::Ok;

    const auto expanded = engine.expand({baseWeekly()}, window, "Etc/UTC", state);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok), static_cast<uint8_t>(state));
    TEST_ASSERT_EQUAL_UINT32(4, expanded.size());
    TEST_ASSERT_EQUAL_INT64(1784552400LL + 7LL * 86400LL, expanded[1].startUtc);
}

void test_exdate_removes_matching_instance_and_rdate_adds_one() {
    CalendarEvent event = baseWeekly();
    event.rrule = "FREQ=WEEKLY;COUNT=3";
    event.exdate.push_back("20260727T130000Z");
    event.rdate.push_back("20260810T130000Z");
    RecurrenceEngine engine;
    RecurrenceWindow window{1783900800LL, 1786924800LL, 1784552400LL, 250};
    SourceState state = SourceState::Ok;

    const auto expanded = engine.expand({event}, window, "Etc/UTC", state);

    TEST_ASSERT_EQUAL_UINT32(3, expanded.size());
    TEST_ASSERT_EQUAL_INT64(1784552400LL, expanded[0].startUtc);
    TEST_ASSERT_EQUAL_INT64(1785762000LL, expanded[2].startUtc);
}

void test_override_replaces_instance_and_cancelled_removes_it() {
    CalendarEvent master = baseWeekly();
    master.rrule = "FREQ=WEEKLY;COUNT=3";
    CalendarEvent override = master;
    override.rrule.clear();
    override.summary = "Moved Sync";
    override.recurrenceId = "20260727T130000Z";
    override.startUtc = 1785160800LL;
    override.endUtc = 1785164400LL;
    override.sequence = 2;
    CalendarEvent cancelled = master;
    cancelled.rrule.clear();
    cancelled.recurrenceId = "20260803T130000Z";
    cancelled.status = CalendarEventStatus::Cancelled;
    cancelled.sequence = 3;
    RecurrenceEngine engine;
    RecurrenceWindow window{1783900800LL, 1786924800LL, 1784552400LL, 250};
    SourceState state = SourceState::Ok;

    const auto expanded = engine.expand({master, override, cancelled}, window, "Etc/UTC", state);

    TEST_ASSERT_EQUAL_UINT32(2, expanded.size());
    TEST_ASSERT_EQUAL_STRING("Moved Sync", expanded[1].summary.c_str());
    TEST_ASSERT_EQUAL_INT64(1785160800LL, expanded[1].startUtc);
}

void test_global_capacity_sets_limit_state() {
    CalendarEvent event = baseWeekly();
    event.rrule = "FREQ=DAILY;COUNT=10";
    RecurrenceEngine engine;
    RecurrenceWindow window{1783900800LL, 1786924800LL, 1784552400LL, 3};
    SourceState state = SourceState::Ok;

    const auto expanded = engine.expand({event}, window, "Etc/UTC", state);

    TEST_ASSERT_EQUAL_UINT32(3, expanded.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Limit), static_cast<uint8_t>(state));
}

void test_monthly_bymonthday_expands_fixed_month_dates() {
    CalendarEvent event;
    event.sourceId = "calendar00";
    event.uid = "monthly-fixed@example.com";
    event.summary = "Monthly bill";
    event.startUtc = 1784106000LL;
    event.endUtc = 1784109600LL;
    event.rrule = "FREQ=MONTHLY;COUNT=3;BYMONTHDAY=15";
    RecurrenceEngine engine;
    RecurrenceWindow window{1783900800LL, 1790800000LL, 1784106000LL, 250};
    SourceState state = SourceState::Ok;

    const auto expanded = engine.expand({event}, window, "Etc/UTC", state);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok), static_cast<uint8_t>(state));
    TEST_ASSERT_EQUAL_UINT32(3, expanded.size());
    TEST_ASSERT_EQUAL_INT64(1784106000LL, expanded[0].startUtc);
    TEST_ASSERT_EQUAL_INT64(1786784400LL, expanded[1].startUtc);
    TEST_ASSERT_EQUAL_INT64(1789462800LL, expanded[2].startUtc);
}

void test_monthly_bysetpos_expands_last_weekday_of_month() {
    CalendarEvent event;
    event.sourceId = "calendar00";
    event.uid = "monthly-last-weekday@example.com";
    event.summary = "Monthly close";
    event.startUtc = 1785488400LL;
    event.endUtc = 1785492000LL;
    event.rrule = "FREQ=MONTHLY;COUNT=3;BYDAY=MO,TU,WE,TH,FR;BYSETPOS=-1";
    RecurrenceEngine engine;
    RecurrenceWindow window{1783900800LL, 1790800000LL, 1785488400LL, 250};
    SourceState state = SourceState::Ok;

    const auto expanded = engine.expand({event}, window, "Etc/UTC", state);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok), static_cast<uint8_t>(state));
    TEST_ASSERT_EQUAL_UINT32(3, expanded.size());
    TEST_ASSERT_EQUAL_INT64(1785488400LL, expanded[0].startUtc);
    TEST_ASSERT_EQUAL_INT64(1788166800LL, expanded[1].startUtc);
    TEST_ASSERT_EQUAL_INT64(1790758800LL, expanded[2].startUtc);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_weekly_rule_expands_count_in_window);
    RUN_TEST(test_exdate_removes_matching_instance_and_rdate_adds_one);
    RUN_TEST(test_override_replaces_instance_and_cancelled_removes_it);
    RUN_TEST(test_global_capacity_sets_limit_state);
    RUN_TEST(test_monthly_bymonthday_expands_fixed_month_dates);
    RUN_TEST(test_monthly_bysetpos_expands_last_weekday_of_month);
    UNITY_END();
}

void loop() {}
