#include <unity.h>

#include <vector>

#include "app/calendar/calendar_models.h"
#include "app/calendar/ics_line_reader.h"
#include "app/calendar/ics_parser.h"
#include "app/calendar/calendar_page_adapter.h"
#include "app/calendar/calendar_models.cpp"
#include "app/calendar/ics_line_reader.cpp"
#include "app/calendar/ics_parser.cpp"
#include "app/calendar/timezone_resolver.cpp"
#include "app/calendar/calendar_page_adapter.cpp"

namespace {
class CollectingSink final : public IcsEventSink {
public:
    bool onEvent(const CalendarEvent &event) override {
        events.push_back(event);
        return true;
    }

    std::vector<CalendarEvent> events;
};
}  // namespace

void test_google_ical_event_reaches_calendar_page_snapshot() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "VERSION:2.0\r\n"
        "PRODID:-//Google Inc//Google Calendar 70.9054//EN\r\n"
        "BEGIN:VEVENT\r\n"
        "UID:google-dashboard-1@example.com\r\n"
        "DTSTAMP:20260722T120000Z\r\n"
        "DTSTART:20260723T013000Z\r\n"
        "DTEND:20260723T020000Z\r\n"
        "SUMMARY:Design review\r\n"
        "LOCATION:Room A\\, Chengdu\r\n"
        "STATUS:CONFIRMED\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";

    CollectingSink sink;
    StringIcsByteReader reader(ics);
    IcsParser parser;
    const IcsParseResult parsed = parser.parse("cal00", reader, sink);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok),
                            static_cast<uint8_t>(parsed.state));
    TEST_ASSERT_EQUAL_UINT32(1, parsed.eventCount);

    const CalendarPageSnapshot snapshot = calendarPageSnapshotFromEvents(
        sink.events, 1784779200LL, "Asia/Shanghai", true);

    TEST_ASSERT_FALSE(snapshot.agendaItems.empty());
    TEST_ASSERT_EQUAL_STRING("09:30  Design review", snapshot.agendaItems[0].c_str());
    TEST_ASSERT_FALSE(snapshot.overviewItems.empty());
    TEST_ASSERT_EQUAL_STRING("09:30", snapshot.overviewItems[0].time.c_str());
    TEST_ASSERT_EQUAL_STRING("Design review", snapshot.overviewItems[0].title.c_str());
    TEST_ASSERT_EQUAL_STRING("Room A, Chengdu", snapshot.overviewItems[0].detail.c_str());
    TEST_ASSERT_EQUAL_INT(0, snapshot.overviewItems[0].dayIndex);
}

void test_google_ical_events_from_current_week_remain_visible_after_event_day() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "VERSION:2.0\r\n"
        "PRODID:-//Google Inc//Google Calendar 70.9054//EN\r\n"
        "BEGIN:VEVENT\r\n"
        "UID:google-dashboard-week-1@example.com\r\n"
        "DTSTAMP:20260722T120000Z\r\n"
        "DTSTART:20260720T020000Z\r\n"
        "DTEND:20260720T030000Z\r\n"
        "SUMMARY:nm-epd-420 coding\r\n"
        "STATUS:CONFIRMED\r\n"
        "END:VEVENT\r\n"
        "BEGIN:VEVENT\r\n"
        "UID:google-dashboard-week-2@example.com\r\n"
        "DTSTAMP:20260722T120000Z\r\n"
        "DTSTART:20260723T013000Z\r\n"
        "DTEND:20260723T020000Z\r\n"
        "SUMMARY:tennis\r\n"
        "STATUS:CONFIRMED\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";

    CollectingSink sink;
    StringIcsByteReader reader(ics);
    IcsParser parser;
    const IcsParseResult parsed = parser.parse("cal00", reader, sink);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok),
                            static_cast<uint8_t>(parsed.state));
    TEST_ASSERT_EQUAL_UINT32(2, parsed.eventCount);

    const CalendarPageSnapshot snapshot = calendarPageSnapshotFromEvents(
        sink.events, 1784858400LL, "Asia/Shanghai", true);

    TEST_ASSERT_EQUAL_STRING("JULY 2026", snapshot.dateTitle.c_str());
    TEST_ASSERT_EQUAL_STRING("JUL 20-26 2026", snapshot.weekRangeLabel.c_str());
    TEST_ASSERT_EQUAL_UINT32(7, snapshot.weekCells.size());
    TEST_ASSERT_EQUAL_STRING("24", snapshot.weekCells[4].text.c_str());
    TEST_ASSERT_TRUE(snapshot.weekCells[4].today);
    TEST_ASSERT_EQUAL_UINT32(2, snapshot.timelineItems.size());
    TEST_ASSERT_EQUAL_INT(0, snapshot.timelineItems[0].dayIndex);
    TEST_ASSERT_EQUAL_STRING("nm-epd-420 coding", snapshot.timelineItems[0].title.c_str());
    TEST_ASSERT_EQUAL_INT(3, snapshot.timelineItems[1].dayIndex);
    TEST_ASSERT_EQUAL_STRING("tennis", snapshot.timelineItems[1].title.c_str());
    TEST_ASSERT_EQUAL_STRING("No calendar events today", snapshot.agendaItems[0].c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_google_ical_event_reaches_calendar_page_snapshot);
    RUN_TEST(test_google_ical_events_from_current_week_remain_visible_after_event_day);
    UNITY_END();
}

void loop() {}
