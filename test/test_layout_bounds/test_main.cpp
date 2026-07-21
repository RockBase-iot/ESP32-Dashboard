#include <unity.h>

#include <algorithm>
#include <array>

#include "ui/canvas/draw_surface.h"
#include "ui/canvas/draw_surface.cpp"
#include "ui/components/calm_grid.h"
#include "ui/components/calm_grid.cpp"
#include "app/time/timezone_catalog.h"
#include "app/time/timezone_catalog.cpp"
#include "app/time/world_clock_model.h"
#include "app/time/world_clock_model.cpp"
#include "ui/layouts/epd_400x300/render_overview.h"
#include "ui/layouts/epd_400x300/render_calendar.h"
#include "ui/layouts/epd_400x300/render_agenda.h"
#include "ui/layouts/epd_400x300/render_weather.h"
#include "ui/layouts/epd_400x300/render_time.h"
#include "ui/layouts/epd_400x300/render_news.h"
#include "ui/layouts/epd_400x300/render_finance.h"
#include "ui/layouts/epd_400x300/render_overview.cpp"
#include "ui/layouts/epd_400x300/render_calendar.cpp"
#include "ui/layouts/epd_400x300/render_agenda.cpp"
#include "ui/layouts/epd_400x300/render_weather.cpp"
#include "ui/layouts/epd_400x300/render_time.cpp"
#include "ui/layouts/epd_400x300/render_news.cpp"
#include "ui/layouts/epd_400x300/render_finance.cpp"

namespace {
void assert_no_layout_faults(const MemoryDrawSurface &surface) {
    TEST_ASSERT_EQUAL_UINT32(0, surface.outOfBoundsCount());
    TEST_ASSERT_EQUAL_UINT32(0, surface.invalidColorCount());
}

void assert_has_text(const MemoryDrawSurface &surface, const char *expected) {
    const bool found = std::any_of(surface.textOps().begin(), surface.textOps().end(),
                                   [expected](const MemoryDrawSurface::TextOp &op) {
                                       return op.text == expected;
                                   });
    TEST_ASSERT_TRUE_MESSAGE(found, expected);
}

const MemoryDrawSurface::TextOp *find_text(const MemoryDrawSurface &surface, const char *expected) {
    const auto it = std::find_if(surface.textOps().begin(), surface.textOps().end(),
                                 [expected](const MemoryDrawSurface::TextOp &op) {
                                     return op.text == expected;
                                 });
    return it == surface.textOps().end() ? nullptr : &(*it);
}

size_t count_text(const MemoryDrawSurface &surface, const char *expected) {
    return static_cast<size_t>(std::count_if(
        surface.textOps().begin(), surface.textOps().end(),
        [expected](const MemoryDrawSurface::TextOp &op) { return op.text == expected; }));
}

bool has_top_right_text(const MemoryDrawSurface &surface) {
    return std::any_of(surface.textOps().begin(), surface.textOps().end(),
                       [](const MemoryDrawSurface::TextOp &op) {
                           return op.y <= 38 && op.align == TextAlign::Right;
                       });
}

int16_t text_left(const MemoryDrawSurface &surface, const MemoryDrawSurface::TextOp &op) {
    const int16_t width = surface.measureText(op.text, op.size);
    if (op.align == TextAlign::Center) {
        return static_cast<int16_t>(op.x - width / 2);
    }
    if (op.align == TextAlign::Right) {
        return static_cast<int16_t>(op.x - width);
    }
    return op.x;
}

int16_t text_right(const MemoryDrawSurface &surface, const MemoryDrawSurface::TextOp &op) {
    return static_cast<int16_t>(text_left(surface, op) + surface.measureText(op.text, op.size));
}

int16_t text_top(const MemoryDrawSurface::TextOp &op) {
    return static_cast<int16_t>(op.y - 8 * op.size + 1);
}

bool text_intersects(const MemoryDrawSurface &surface,
                     const MemoryDrawSurface::TextOp &lhs,
                     const MemoryDrawSurface::TextOp &rhs) {
    if (lhs.text.empty() || rhs.text.empty()) {
        return false;
    }
    const int16_t left = std::max(text_left(surface, lhs), text_left(surface, rhs));
    const int16_t right = std::min(text_right(surface, lhs), text_right(surface, rhs));
    const int16_t top = std::max(text_top(lhs), text_top(rhs));
    const int16_t bottom = std::min(lhs.y, rhs.y);
    return left < right && top < bottom;
}

void assert_no_text_overlap(const MemoryDrawSurface &surface) {
    const auto &ops = surface.textOps();
    for (size_t i = 0; i < ops.size(); ++i) {
        for (size_t j = i + 1; j < ops.size(); ++j) {
            if (text_intersects(surface, ops[i], ops[j])) {
                TEST_FAIL_MESSAGE(("Text overlap: " + ops[i].text + " / " + ops[j].text).c_str());
            }
        }
    }
}

void assert_all_body_text_below_header(const MemoryDrawSurface &surface, int16_t minY) {
    for (const auto &op : surface.textOps()) {
        if (op.y > 38) {
            TEST_ASSERT_GREATER_OR_EQUAL(minY, op.y);
        }
    }
}

void assert_header_has_clear_status(const MemoryDrawSurface &surface) {
    const auto *wifi = find_text(surface, "WiFi:192.168.1.42");
    const auto *battery = find_text(surface, "BAT:82%");
    TEST_ASSERT_NOT_NULL(wifi);
    TEST_ASSERT_NOT_NULL(battery);
    TEST_ASSERT_LESS_THAN(280, text_left(surface, *wifi));
    TEST_ASSERT_GREATER_OR_EQUAL(296, text_left(surface, *battery));
    TEST_ASSERT_LESS_OR_EQUAL(382, text_right(surface, *battery));
}
}  // namespace

void test_calendar_pages_stay_inside_400x300() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();

    {
        MemoryDrawSurface surface(400, 300);
        renderOverviewPage(surface, snapshot);
        assert_no_layout_faults(surface);
        assert_has_text(surface, "TODAY OVERVIEW");
        assert_has_text(surface, "Design review");
        assert_has_text(surface, "LOCAL NOTE");
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderMonthlyOverviewPage(surface, snapshot);
        assert_no_layout_faults(surface);
        assert_has_text(surface, "JULY 2026");
        assert_has_text(surface, "Review");
        TEST_ASSERT_NULL(find_text(surface, "12 events"));
        TEST_ASSERT_NULL(find_text(surface, "6/6"));
        TEST_ASSERT_NULL(find_text(surface, "2/12"));
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderWeeklyTimelinePage(surface, snapshot);
        assert_no_layout_faults(surface);
        assert_has_text(surface, "WEEKLY TIMELINE");
        assert_has_text(surface, "NEXT 09:00 - Design review");
        assert_has_text(surface, "JUL 20-26 2026   3/12");
        TEST_ASSERT_NULL(find_text(surface, "JUL 20-26   6/6   3/12"));
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderTodayAgendaPage(surface, snapshot);
        renderLocalNotesPage(surface, snapshot);
        renderImportantMilestonesPage(surface, snapshot);
        assert_no_layout_faults(surface);
    }
}

void test_today_overview_matches_compact_prototype_layout() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderOverviewPage(surface, snapshot);

    assert_no_layout_faults(surface);
    const auto *title = find_text(surface, "TODAY OVERVIEW");
    const auto *status = find_text(surface, "WiFi:192.168.1.42");
    const auto *battery = find_text(surface, "BAT:82%");
    const auto *noteTitle = find_text(surface, "LOCAL NOTE");
    TEST_ASSERT_NOT_NULL(title);
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_NOT_NULL(battery);
    TEST_ASSERT_NOT_NULL(noteTitle);
    TEST_ASSERT_FALSE(text_intersects(surface, *title, *status));
    TEST_ASSERT_FALSE(text_intersects(surface, *title, *battery));
    TEST_ASSERT_LESS_OR_EQUAL(382, text_right(surface, *noteTitle));

    const auto *detail = find_text(surface, "Office - Project Atlas");
    TEST_ASSERT_NOT_NULL(detail);
    TEST_ASSERT_LESS_OR_EQUAL(300, text_right(surface, *detail));
}

void test_monthly_overview_uses_five_by_seven_grid() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderMonthlyOverviewPage(surface, snapshot);

    assert_no_layout_faults(surface);
    const auto *title = find_text(surface, "JULY 2026");
    TEST_ASSERT_NOT_NULL(title);
    TEST_ASSERT_NULL(find_text(surface, "12 events"));
    TEST_ASSERT_NULL(find_text(surface, "6/6"));
    TEST_ASSERT_NULL(find_text(surface, "2/12"));
    TEST_ASSERT_NULL(find_text(surface, "27"));
}

void test_weekly_timeline_has_clear_header_and_roomy_event_cards() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderWeeklyTimelinePage(surface, snapshot);

    assert_no_layout_faults(surface);
    assert_no_text_overlap(surface);
    assert_has_text(surface, "JUL 20-26 2026   3/12");
    TEST_ASSERT_NULL(find_text(surface, "6/6"));

    const auto *mon = find_text(surface, "MON");
    const auto *day = find_text(surface, "20");
    const auto *time = find_text(surface, "09:00");
    const auto *title = find_text(surface, "Review");
    TEST_ASSERT_NOT_NULL(mon);
    TEST_ASSERT_NOT_NULL(day);
    TEST_ASSERT_NOT_NULL(time);
    TEST_ASSERT_NOT_NULL(title);
    TEST_ASSERT_EQUAL_UINT16(kDashboardAccent, mon->color);
    TEST_ASSERT_EQUAL_UINT16(kDashboardAccent, day->color);
    TEST_ASSERT_EQUAL_UINT16(kDashboardAccent, time->color);
    TEST_ASSERT_GREATER_THAN(time->y, title->y);
}

void test_today_agenda_uses_ascii_text_and_clear_status() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderTodayAgendaPage(surface, snapshot);

    assert_no_layout_faults(surface);
    assert_has_text(surface, "Today Agenda");
    assert_header_has_clear_status(surface);
    TEST_ASSERT_NULL(find_text(surface, "\u2022"));

    const auto *firstAgenda = find_text(surface, "09:00  User review");
    const auto *firstNote = find_text(surface, "Buy milk");
    const auto *firstMilestone = find_text(surface, "Task 8 recurrence");
    TEST_ASSERT_NOT_NULL(firstAgenda);
    TEST_ASSERT_NOT_NULL(firstNote);
    TEST_ASSERT_NOT_NULL(firstMilestone);
    TEST_ASSERT_GREATER_OR_EQUAL(28, text_left(surface, *firstAgenda));
    TEST_ASSERT_GREATER_OR_EQUAL(28, text_left(surface, *firstNote));
    TEST_ASSERT_GREATER_OR_EQUAL(224, text_left(surface, *firstMilestone));
}

void test_epd_pages_do_not_overlap_text_or_header() {
    const std::array<void (*)(MemoryDrawSurface &), 10> renderers = {{
        [](MemoryDrawSurface &surface) { renderTodayAgendaPage(surface, sampleCalendarPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderImportantMilestonesPage(surface, sampleCalendarPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderHeadlinesPage(surface, sampleNewsPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderTodayInHistoryPage(surface, sampleNewsPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderWorldClockPage(surface, sampleWorldClockPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderFocusClockPage(surface, sampleWorldClockPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderWeatherTodayPage(surface, sampleWeatherPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderWeeklyWeatherPage(surface, sampleWeatherPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderPortfolioSummaryPage(surface, sampleFinancePageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderEconomicCalendarPage(surface, sampleFinancePageSnapshot()); },
    }};

    for (auto renderer : renderers) {
        MemoryDrawSurface surface(400, 300);
        renderer(surface);
        assert_no_layout_faults(surface);
        assert_no_text_overlap(surface);
        assert_all_body_text_below_header(surface, 52);
    }
}

void test_pages_reset_font_state_between_renderers() {
    MemoryDrawSurface weatherSurface(400, 300);
    renderWeatherTodayPage(weatherSurface, sampleWeatherPageSnapshot());
    assert_no_layout_faults(weatherSurface);

    MemoryDrawSurface surface(400, 300);
    renderImportantMilestonesPage(surface, sampleCalendarPageSnapshot());

    assert_no_layout_faults(surface);
    assert_has_text(surface, "Important Milestones");
    assert_header_has_clear_status(surface);
    assert_no_text_overlap(surface);

    const auto *title = find_text(surface, "Important Milestones");
    TEST_ASSERT_NOT_NULL(title);
    const auto *status = find_text(surface, "WiFi:192.168.1.42");
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_FALSE(text_intersects(surface, *title, *status));
}

void test_weather_time_and_news_pages_match_prototype_text() {
    {
        MemoryDrawSurface surface(400, 300);
        renderWeatherTodayPage(surface, sampleWeatherPageSnapshot());
        assert_no_layout_faults(surface);
        assert_has_text(surface, "WEATHER TODAY");
        assert_has_text(surface, "Sunny");
        assert_has_text(surface, "Rain");
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderWorldClockPage(surface, sampleWorldClockPageSnapshot());
        assert_no_layout_faults(surface);
        assert_has_text(surface, "WORLD CLOCK");
        assert_has_text(surface, "MON JUL 20, 2026   4/12");
        assert_has_text(surface, "08:42");
        assert_has_text(surface, "SHANGHAI");
        TEST_ASSERT_NULL(find_text(surface, "Updated by SNTP"));
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderHeadlinesPage(surface, sampleNewsPageSnapshot());
        assert_no_layout_faults(surface);
        assert_has_text(surface, "HEADLINES");
        assert_has_text(surface, "1969");
        assert_has_text(surface, "Today in History");
    }
}

void test_world_clock_keeps_title_left_and_separates_card_text() {
    const WorldClockPageSnapshot snapshot = sampleWorldClockPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderWorldClockPage(surface, snapshot);

    const auto *pageTitle = find_text(surface, "WORLD CLOCK");
    const auto *dateMeta = find_text(surface, "MON JUL 20, 2026   4/12");
    const auto *cityTitle = find_text(surface, "NEW YORK");
    const auto *timeText = find_text(surface, "08:42");
    const auto *zoneStatus = find_text(surface, "EDT - WORKING");
    TEST_ASSERT_NOT_NULL(pageTitle);
    TEST_ASSERT_NOT_NULL(dateMeta);
    TEST_ASSERT_NOT_NULL(cityTitle);
    TEST_ASSERT_NOT_NULL(timeText);
    TEST_ASSERT_NOT_NULL(zoneStatus);
    TEST_ASSERT_EQUAL_UINT(1, count_text(surface, "WORLD CLOCK"));
    TEST_ASSERT_FALSE(has_top_right_text(surface));
    TEST_ASSERT_EQUAL_INT16(28, dateMeta->x);
    TEST_ASSERT_EQUAL_INT16(14, dateMeta->y);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TextAlign::Left),
                            static_cast<uint8_t>(dateMeta->align));
    TEST_ASSERT_LESS_THAN(220, text_right(surface, *dateMeta));
    TEST_ASSERT_EQUAL_INT16(28, pageTitle->x);
    TEST_ASSERT_EQUAL_INT16(31, pageTitle->y);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TextAlign::Left),
                            static_cast<uint8_t>(pageTitle->align));
    TEST_ASSERT_EQUAL_INT16(26, cityTitle->x);
    TEST_ASSERT_EQUAL_INT16(68, cityTitle->y);
    TEST_ASSERT_EQUAL_INT16(92, timeText->y);
    TEST_ASSERT_EQUAL_INT16(126, zoneStatus->y);

    WorldClockPageSnapshot duplicateMetadata = snapshot;
    duplicateMetadata.subtitle = duplicateMetadata.title;
    MemoryDrawSurface duplicateSurface(400, 300);
    renderWorldClockPage(duplicateSurface, duplicateMetadata);
    TEST_ASSERT_NULL(find_text(duplicateSurface, "WORLD CLOCK   4/12"));
    assert_has_text(duplicateSurface, "4/12");
}

void test_weather_time_and_news_pages_stay_inside_400x300() {
    MemoryDrawSurface surface(400, 300);

    renderWeatherTodayPage(surface, sampleWeatherPageSnapshot());
    renderWeeklyWeatherPage(surface, sampleWeatherPageSnapshot());
    renderIndoorClimatePage(surface, sampleWeatherPageSnapshot());
    renderWorldClockPage(surface, sampleWorldClockPageSnapshot());
    renderFocusClockPage(surface, sampleWorldClockPageSnapshot());
    renderHeadlinesPage(surface, sampleNewsPageSnapshot());
    renderTodayInHistoryPage(surface, sampleNewsPageSnapshot());

    assert_no_layout_faults(surface);
}

void test_finance_pages_match_prototype_text_and_stay_inside_400x300() {
    MemoryDrawSurface surface(400, 300);
    const FinancePageSnapshot snapshot = sampleFinancePageSnapshot();

    renderStockInfoPage(surface, snapshot);
    renderPortfolioSummaryPage(surface, snapshot);
    renderEconomicCalendarPage(surface, snapshot);

    assert_no_layout_faults(surface);
    assert_has_text(surface, "STOCK INFO");
    assert_has_text(surface, "PORTFOLIO");
    assert_has_text(surface, "ECONOMIC CALENDAR");
    assert_has_text(surface, "Delayed");
    assert_has_text(surface, "Source");
    assert_has_text(surface, "Updated");
    assert_has_text(surface, "Not investment advice");
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_calendar_pages_stay_inside_400x300);
    RUN_TEST(test_today_overview_matches_compact_prototype_layout);
    RUN_TEST(test_monthly_overview_uses_five_by_seven_grid);
    RUN_TEST(test_weekly_timeline_has_clear_header_and_roomy_event_cards);
    RUN_TEST(test_today_agenda_uses_ascii_text_and_clear_status);
    RUN_TEST(test_epd_pages_do_not_overlap_text_or_header);
    RUN_TEST(test_pages_reset_font_state_between_renderers);
    RUN_TEST(test_weather_time_and_news_pages_match_prototype_text);
    RUN_TEST(test_world_clock_keeps_title_left_and_separates_card_text);
    RUN_TEST(test_weather_time_and_news_pages_stay_inside_400x300);
    RUN_TEST(test_finance_pages_match_prototype_text_and_stay_inside_400x300);
    UNITY_END();
}

void loop() {}
