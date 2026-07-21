#include <unity.h>

#include <vector>

#include "app/calendar/calendar_models.h"
#include "app/calendar/ics_line_reader.h"
#include "app/calendar/ics_parser.h"
#include "app/calendar/calendar_models.cpp"
#include "app/calendar/ics_line_reader.cpp"
#include "app/calendar/ics_parser.cpp"

namespace {
class CollectingSink final : public IcsEventSink {
public:
    bool onEvent(const CalendarEvent &event) override {
        events.push_back(event);
        return true;
    }

    std::vector<CalendarEvent> events;
};

IcsParseResult parseText(const std::string &sourceId, const std::string &ics,
                         CollectingSink &sink) {
    StringIcsByteReader bytes(ics);
    IcsParser parser;
    return parser.parse(sourceId, bytes, sink);
}
}  // namespace

void test_google_fixture_parses_folded_escaped_utc_event() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "VERSION:2.0\r\n"
        "BEGIN:VEVENT\r\n"
        "UID:google-1@example.com\r\n"
        "DTSTAMP:20260718T010203Z\r\n"
        "DTSTART:20260718T020000Z\r\n"
        "DTEND:20260718T030000Z\r\n"
        "SUMMARY:Design review with very long\r\n"
        " continuation\r\n"
        "LOCATION:Room A\\, Chengdu\r\n"
        "DESCRIPTION:Line one\\nLine two\\; bring docs\r\n"
        "SEQUENCE:4\r\n"
        "STATUS:CONFIRMED\r\n"
        "X-GOOGLE-CUSTOM:ignored\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    CollectingSink sink;

    const auto result = parseText("calendar00", ics, sink);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok),
                            static_cast<uint8_t>(result.state));
    TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
    TEST_ASSERT_EQUAL_UINT32(0, result.skippedCount);
    TEST_ASSERT_EQUAL_STRING("calendar00", sink.events[0].sourceId.c_str());
    TEST_ASSERT_EQUAL_STRING("google-1@example.com", sink.events[0].uid.c_str());
    TEST_ASSERT_EQUAL_STRING("Design review with very longcontinuation", sink.events[0].summary.c_str());
    TEST_ASSERT_EQUAL_STRING("Room A, Chengdu", sink.events[0].location.c_str());
    TEST_ASSERT_EQUAL_STRING("Line one\nLine two; bring docs", sink.events[0].description.c_str());
    TEST_ASSERT_EQUAL_UINT32(4, sink.events[0].sequence);
    TEST_ASSERT_FALSE(sink.events[0].allDay);
    TEST_ASSERT_EQUAL_INT64(1784311200LL, sink.events[0].startUtc);
    TEST_ASSERT_EQUAL_INT64(1784314800LL, sink.events[0].endUtc);
}

void test_outlook_fixture_parses_tzid_floating_and_duration() {
    const std::string ics =
        "BEGIN:VCALENDAR\n"
        "VERSION:2.0\n"
        "BEGIN:VEVENT\n"
        "UID:outlook-1@example.com\n"
        "DTSTAMP:20260718T041500Z\n"
        "DTSTART;TZID=Asia/Shanghai:20260720T090000\n"
        "DURATION:PT45M\n"
        "SUMMARY:Standup\n"
        "TRANSP:OPAQUE\n"
        "ATTENDEE;CN=Someone:mailto:test@example.com\n"
        "BEGIN:VALARM\n"
        "TRIGGER:-PT10M\n"
        "END:VALARM\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    CollectingSink sink;

    const auto result = parseText("calendar01", ics, sink);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok),
                            static_cast<uint8_t>(result.state));
    TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
    TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", sink.events[0].startTzid.c_str());
    TEST_ASSERT_TRUE(sink.events[0].startFloating);
    TEST_ASSERT_EQUAL_INT64(1784509200LL, sink.events[0].startUtc);
    TEST_ASSERT_EQUAL_INT64(1784511900LL, sink.events[0].endUtc);
}

void test_apple_fixture_parses_all_day_status_and_recurrence_fields() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "VERSION:2.0\r\n"
        "BEGIN:VEVENT\r\n"
        "UID:apple-1@example.com\r\n"
        "RECURRENCE-ID;VALUE=DATE:20260721\r\n"
        "DTSTAMP:20260718T050000Z\r\n"
        "DTSTART;VALUE=DATE:20260721\r\n"
        "DTEND;VALUE=DATE:20260722\r\n"
        "SUMMARY:Travel day\r\n"
        "STATUS:CANCELLED\r\n"
        "RRULE:FREQ=WEEKLY;COUNT=3\r\n"
        "EXDATE;VALUE=DATE:20260728\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    CollectingSink sink;

    const auto result = parseText("calendar02", ics, sink);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok),
                            static_cast<uint8_t>(result.state));
    TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
    TEST_ASSERT_TRUE(sink.events[0].allDay);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CalendarEventStatus::Cancelled),
                            static_cast<uint8_t>(sink.events[0].status));
    TEST_ASSERT_EQUAL_STRING("20260721", sink.events[0].recurrenceId.c_str());
    TEST_ASSERT_EQUAL_STRING("FREQ=WEEKLY;COUNT=3", sink.events[0].rrule.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, sink.events[0].exdate.size());
    TEST_ASSERT_EQUAL_STRING("20260728", sink.events[0].exdate[0].c_str());
}

void test_malformed_event_is_skipped_and_later_event_survives() {
    const std::string ics =
        "BEGIN:VCALENDAR\n"
        "VERSION:2.0\n"
        "BEGIN:VEVENT\n"
        "UID:bad-event@example.com\n"
        "SUMMARY:No start date\n"
        "END:VEVENT\n"
        "BEGIN:VEVENT\n"
        "UID:good-event@example.com\n"
        "DTSTART:20260722T120000Z\n"
        "DTEND:20260722T130000Z\n"
        "SUMMARY:Recovered\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n";
    CollectingSink sink;

    const auto result = parseText("calendar03", ics, sink);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok),
                            static_cast<uint8_t>(result.state));
    TEST_ASSERT_EQUAL_UINT32(1, result.eventCount);
    TEST_ASSERT_EQUAL_UINT32(1, result.skippedCount);
    TEST_ASSERT_EQUAL_STRING("good-event@example.com", sink.events[0].uid.c_str());
}

void test_source_without_vcalendar_returns_parse_error() {
    const std::string ics =
        "BEGIN:VEVENT\n"
        "UID:no-calendar@example.com\n"
        "DTSTART:20260722T120000Z\n"
        "END:VEVENT\n";
    CollectingSink sink;

    const auto result = parseText("calendar04", ics, sink);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Parse),
                            static_cast<uint8_t>(result.state));
    TEST_ASSERT_EQUAL_UINT32(0, result.eventCount);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_google_fixture_parses_folded_escaped_utc_event);
    RUN_TEST(test_outlook_fixture_parses_tzid_floating_and_duration);
    RUN_TEST(test_apple_fixture_parses_all_day_status_and_recurrence_fields);
    RUN_TEST(test_malformed_event_is_skipped_and_later_event_survives);
    RUN_TEST(test_source_without_vcalendar_returns_parse_error);
    UNITY_END();
}

void loop() {}
