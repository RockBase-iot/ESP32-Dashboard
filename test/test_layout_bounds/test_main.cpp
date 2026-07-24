#include <unity.h>

#include <algorithm>
#include <array>
#include <utility>

#include "ui/canvas/draw_surface.h"
#include "ui/canvas/draw_surface.cpp"
#include "ui/components/calm_grid.h"
#include "ui/components/calm_grid.cpp"
#include "app/time/timezone_catalog.h"
#include "app/time/timezone_catalog.cpp"
#include "app/time/focus_clock_model.h"
#include "app/time/focus_clock_model.cpp"
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
class IconProbeSurface final : public IDrawSurface {
public:
    int16_t width() const override { return 400; }
    int16_t height() const override { return 300; }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (color != kDashboardBlack) {
            return;
        }
        if (std::any_of(_pixels.begin(), _pixels.end(),
                        [x, y](const auto &pixel) { return pixel.first == x && pixel.second == y; })) {
            return;
        }
        _pixels.push_back({x, y});
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override {
        int16_t dx = std::abs(static_cast<int>(x1 - x0));
        int16_t sx = x0 < x1 ? 1 : -1;
        int16_t dy = -std::abs(static_cast<int>(y1 - y0));
        int16_t sy = y0 < y1 ? 1 : -1;
        int16_t err = static_cast<int16_t>(dx + dy);
        while (true) {
            drawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) {
                break;
            }
            const int16_t e2 = static_cast<int16_t>(2 * err);
            if (e2 >= dy) {
                err = static_cast<int16_t>(err + dy);
                x0 = static_cast<int16_t>(x0 + sx);
            }
            if (e2 <= dx) {
                err = static_cast<int16_t>(err + dx);
                y0 = static_cast<int16_t>(y0 + sy);
            }
        }
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        fillRect(x, y, w, 1, color);
        fillRect(x, static_cast<int16_t>(y + h - 1), w, 1, color);
        fillRect(x, y, 1, h, color);
        fillRect(static_cast<int16_t>(x + w - 1), y, 1, h, color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        for (int16_t yy = 0; yy < h; ++yy) {
            for (int16_t xx = 0; xx < w; ++xx) {
                drawPixel(static_cast<int16_t>(x + xx), static_cast<int16_t>(y + yy), color);
            }
        }
    }

    void fillScreen(uint16_t color) override {}
    void drawText(int16_t, int16_t, const std::string &, uint16_t, TextAlign, uint8_t) override {}
    int16_t measureText(const std::string &text, uint8_t size) const override {
        return static_cast<int16_t>(text.size() * 6 * size);
    }

    int16_t minX() const {
        return std::min_element(_pixels.begin(), _pixels.end(),
                                [](const auto &lhs, const auto &rhs) {
                                    return lhs.first < rhs.first;
                                })->first;
    }

    int16_t maxX() const {
        return std::max_element(_pixels.begin(), _pixels.end(),
                                [](const auto &lhs, const auto &rhs) {
                                    return lhs.first < rhs.first;
                                })->first;
    }

    int16_t maxY() const {
        return std::max_element(_pixels.begin(), _pixels.end(),
                                [](const auto &lhs, const auto &rhs) {
                                    return lhs.second < rhs.second;
                                })->second;
    }

    size_t countInRect(const Rect &rect) const {
        return static_cast<size_t>(std::count_if(
            _pixels.begin(), _pixels.end(),
            [&rect](const auto &pixel) {
                return pixel.first >= rect.x && pixel.first < rect.x + rect.w &&
                       pixel.second >= rect.y && pixel.second < rect.y + rect.h;
            }));
    }

    size_t countLeftOf(int16_t centerX) const {
        return static_cast<size_t>(std::count_if(
            _pixels.begin(), _pixels.end(),
            [centerX](const auto &pixel) { return pixel.first < centerX; }));
    }

    size_t countRightOf(int16_t centerX) const {
        return static_cast<size_t>(std::count_if(
            _pixels.begin(), _pixels.end(),
            [centerX](const auto &pixel) { return pixel.first > centerX; }));
    }

private:
    std::vector<std::pair<int16_t, int16_t>> _pixels;
};

class ColorProbeSurface final : public IDrawSurface {
public:
    explicit ColorProbeSurface(int16_t width, int16_t height) : _width(width), _height(height) {}

    int16_t width() const override { return _width; }
    int16_t height() const override { return _height; }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (x < 0 || y < 0 || x >= _width || y >= _height) {
            return;
        }
        _pixels.push_back(Pixel{x, y, color});
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override {
        int16_t dx = std::abs(static_cast<int>(x1 - x0));
        int16_t sx = x0 < x1 ? 1 : -1;
        int16_t dy = -std::abs(static_cast<int>(y1 - y0));
        int16_t sy = y0 < y1 ? 1 : -1;
        int16_t err = static_cast<int16_t>(dx + dy);
        while (true) {
            drawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) {
                break;
            }
            const int16_t e2 = static_cast<int16_t>(2 * err);
            if (e2 >= dy) {
                err = static_cast<int16_t>(err + dy);
                x0 = static_cast<int16_t>(x0 + sx);
            }
            if (e2 <= dx) {
                err = static_cast<int16_t>(err + dx);
                y0 = static_cast<int16_t>(y0 + sy);
            }
        }
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        fillRect(x, y, w, 1, color);
        fillRect(x, static_cast<int16_t>(y + h - 1), w, 1, color);
        fillRect(x, y, 1, h, color);
        fillRect(static_cast<int16_t>(x + w - 1), y, 1, h, color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        for (int16_t yy = 0; yy < h; ++yy) {
            for (int16_t xx = 0; xx < w; ++xx) {
                drawPixel(static_cast<int16_t>(x + xx), static_cast<int16_t>(y + yy), color);
            }
        }
    }

    void fillScreen(uint16_t color) override {}
    void drawText(int16_t, int16_t, const std::string &, uint16_t, TextAlign, uint8_t) override {}
    int16_t measureText(const std::string &text, uint8_t size) const override {
        return static_cast<int16_t>(text.size() * 6 * size);
    }

    size_t countColorInRect(uint16_t color, const Rect &rect) const {
        return static_cast<size_t>(std::count_if(
            _pixels.begin(), _pixels.end(),
            [color, &rect](const Pixel &pixel) {
                return pixel.color == color &&
                       pixel.x >= rect.x && pixel.x < rect.x + rect.w &&
                       pixel.y >= rect.y && pixel.y < rect.y + rect.h;
            }));
    }

private:
    struct Pixel {
        int16_t x = 0;
        int16_t y = 0;
        uint16_t color = 0;
    };

    int16_t _width;
    int16_t _height;
    std::vector<Pixel> _pixels;
};

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

void assert_prototype_chrome(const MemoryDrawSurface &surface, const char *title,
                             const char *pageIndicator,
                             const char *timeText = "WED 14:32 JUL 22, 2026",
                             const char *ipText = "IP: --",
                             const char *batteryText = "82%") {
    const auto *titleText = find_text(surface, title);
    const auto *battery = find_text(surface, batteryText);
    const auto *time = find_text(surface, timeText);
    const auto *ip = find_text(surface, ipText);
    const auto *page = find_text(surface, pageIndicator);
    TEST_ASSERT_NOT_NULL(titleText);
    TEST_ASSERT_NOT_NULL(battery);
    TEST_ASSERT_NOT_NULL(time);
    TEST_ASSERT_NOT_NULL(ip);
    TEST_ASSERT_NOT_NULL(page);
    TEST_ASSERT_GREATER_THAN_UINT32(0, surface.accentTopLeftIconPixelCount());
    TEST_ASSERT_LESS_OR_EQUAL(42, titleText->x);
    TEST_ASSERT_GREATER_OR_EQUAL(350, text_left(surface, *battery));
    TEST_ASSERT_GREATER_OR_EQUAL(236, text_left(surface, *time));
    TEST_ASSERT_EQUAL_INT16(18, ip->x);
    TEST_ASSERT_EQUAL_INT16(286, ip->y);
    TEST_ASSERT_EQUAL_INT16(382, page->x);
    TEST_ASSERT_EQUAL_INT16(286, page->y);
    TEST_ASSERT_NULL(find_text(surface, "WiFi:192.168.1.42"));
    TEST_ASSERT_NULL(find_text(surface, "IP: 192.168.1.42"));
    TEST_ASSERT_NULL(find_text(surface, "BAT:82%"));
}
}  // namespace

void test_calendar_pages_stay_inside_400x300() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();

    {
        MemoryDrawSurface surface(400, 300);
        renderOverviewPage(surface, snapshot, 1, 8, "IP: 10.0.0.23");
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "TODAY OVERVIEW", "1 | 8");
        assert_has_text(surface, "Design review");
        assert_has_text(surface, "LOCAL NOTE");
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderMonthlyOverviewPage(surface, snapshot, 2, 8, "IP: 10.0.0.23");
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "MONTHLY OVERVIEW", "2 | 8");
        assert_has_text(surface, "JULY 2026");
        assert_has_text(surface, "Review");
        TEST_ASSERT_NULL(find_text(surface, "12 events"));
        TEST_ASSERT_NULL(find_text(surface, "6/6"));
        TEST_ASSERT_NULL(find_text(surface, "2/12"));
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderWeeklyTimelinePage(surface, snapshot, 3, 8, "IP: 10.0.0.23");
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "WEEKLY TIMELINE", "3 | 8");
        assert_has_text(surface, "NEXT 09:00 - Design review");
        assert_has_text(surface, "JUL 20-26 2026");
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
    const auto *noteTitle = find_text(surface, "LOCAL NOTE");
    TEST_ASSERT_NOT_NULL(title);
    TEST_ASSERT_NOT_NULL(noteTitle);
    assert_prototype_chrome(surface, "TODAY OVERVIEW", "1 | 8");
    TEST_ASSERT_LESS_OR_EQUAL(382, text_right(surface, *noteTitle));

    const auto *detail = find_text(surface, "Office - Project Atlas");
    TEST_ASSERT_NOT_NULL(detail);
    TEST_ASSERT_LESS_OR_EQUAL(300, text_right(surface, *detail));
}

void test_prototype_chrome_accepts_runtime_time_battery_and_ip() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();
    calm_grid::ChromeContext chrome;
    chrome.timeText = "THU 10:05 JUL 23, 2026";
    chrome.batteryText = "76%";
    chrome.ipText = "IP: 192.168.1.50";
    MemoryDrawSurface surface(400, 300);

    renderOverviewPage(surface, snapshot, 1, 12, chrome.ipText, chrome);

    assert_prototype_chrome(surface, "TODAY OVERVIEW", "1 | 12",
                            "THU 10:05 JUL 23, 2026", "IP: 192.168.1.50", "76%");
    assert_has_text(surface, "76%");
    TEST_ASSERT_NULL(find_text(surface, "82%"));
    TEST_ASSERT_NULL(find_text(surface, "WED 14:32 JUL 22, 2026"));
}

void test_monthly_overview_uses_event_list_layout() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderMonthlyOverviewPage(surface, snapshot);

    assert_no_layout_faults(surface);
    assert_prototype_chrome(surface, "MONTHLY OVERVIEW", "2 | 8");
    assert_has_text(surface, "JULY 2026");
    TEST_ASSERT_NULL(find_text(surface, "12 events"));
    TEST_ASSERT_NULL(find_text(surface, "6/6"));
    TEST_ASSERT_NULL(find_text(surface, "2/12"));
    TEST_ASSERT_NULL(find_text(surface, "27"));
    assert_has_text(surface, "Birthday");
}

void test_weekly_timeline_has_clear_header_and_roomy_event_cards() {
    CalendarPageSnapshot snapshot = sampleCalendarPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderWeeklyTimelinePage(surface, snapshot);

    assert_no_layout_faults(surface);
    assert_no_text_overlap(surface);
    assert_prototype_chrome(surface, "WEEKLY TIMELINE", "3 | 8");
    assert_has_text(surface, "JUL 20-26 2026");
    TEST_ASSERT_NULL(find_text(surface, "6/6"));

    const auto *mon = find_text(surface, "WED");
    const auto *day = find_text(surface, "22");
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
    assert_prototype_chrome(surface, "TODAY AGENDA", "2 | 8");
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

void test_prototype_wifi_icon_is_compact_and_balanced() {
    IconProbeSurface surface;
    calm_grid::drawPrototypeWifiIcon(surface, 282, 8);

    TEST_ASSERT_GREATER_OR_EQUAL(282, surface.minX());
    TEST_ASSERT_LESS_OR_EQUAL(300, surface.maxX());
    TEST_ASSERT_LESS_OR_EQUAL(25, surface.maxY());
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(4, surface.countInRect(Rect{289, 24, 4, 4}));
    const size_t left = surface.countLeftOf(291);
    const size_t right = surface.countRightOf(291);
    const size_t delta = left > right ? left - right : right - left;
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(3, delta);
}

void test_epd_pages_do_not_overlap_text_or_header() {
    const std::array<void (*)(MemoryDrawSurface &), 10> renderers = {{
        [](MemoryDrawSurface &surface) { renderTodayAgendaPage(surface, sampleCalendarPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderImportantMilestonesPage(surface, sampleCalendarPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderHeadlinesPage(surface, sampleNewsPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderTodayInHistoryPage(surface, sampleNewsPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderWorldClockPage(surface, sampleWorldClockPageSnapshot()); },
        [](MemoryDrawSurface &surface) { renderFocusClockPage(surface, sampleFocusClockPageSnapshot()); },
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
    assert_prototype_chrome(surface, "IMPORTANT MILESTONES", "8 | 8");
    assert_no_text_overlap(surface);

    const auto *title = find_text(surface, "IMPORTANT MILESTONES");
    TEST_ASSERT_NOT_NULL(title);
    const auto *time = find_text(surface, "WED 14:32 JUL 22, 2026");
    TEST_ASSERT_NOT_NULL(time);
    TEST_ASSERT_FALSE(text_intersects(surface, *title, *time));
}

void test_weather_time_and_news_pages_match_prototype_text() {
    {
        MemoryDrawSurface surface(400, 300);
        renderWeatherTodayPage(surface, sampleWeatherPageSnapshot());
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "WEATHER TODAY", "6 | 8");
        assert_has_text(surface, "NEW YORK");
        assert_has_text(surface, "NY");
        assert_has_text(surface, "USA");
        assert_has_text(surface, "Sunny");
        assert_has_text(surface, "HOURLY");
        assert_has_text(surface, "WEEKLY WEATHER");
        assert_has_text(surface, "WED");
        assert_has_text(surface, "SUN");
        assert_has_text(surface, "75/61");
        const auto *city = find_text(surface, "NEW YORK");
        const auto *region = find_text(surface, "NY");
        const auto *country = find_text(surface, "USA");
        const auto *temp = find_text(surface, "72F");
        TEST_ASSERT_NOT_NULL(city);
        TEST_ASSERT_NOT_NULL(region);
        TEST_ASSERT_NOT_NULL(country);
        TEST_ASSERT_NOT_NULL(temp);
        TEST_ASSERT_LESS_THAN(temp->y, city->y);
        TEST_ASSERT_LESS_THAN(temp->y, region->y);
        TEST_ASSERT_LESS_THAN(temp->y, country->y);
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderWeeklyWeatherPage(surface, sampleWeatherPageSnapshot());
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "WEEKLY WEATHER", "6 | 8");
        assert_has_text(surface, "WEEKLY WEATHER");
        assert_has_text(surface, "SAT");
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderIndoorClimatePage(surface, sampleWeatherPageSnapshot());
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "INDOOR CLIMATE", "6 | 8");
        assert_has_text(surface, "INDOOR CLIMATE");
        assert_has_text(surface, "Indoor");
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderWorldClockPage(surface, sampleWorldClockPageSnapshot());
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "WORLD CLOCK", "4 | 8", "MON JUL 20, 2026");
        assert_has_text(surface, "MON JUL 20, 2026");
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

void test_weekly_weather_uses_five_day_trend_layout() {
    MemoryDrawSurface surface(400, 300);

    renderWeeklyWeatherPage(surface, sampleWeatherPageSnapshot());

    assert_no_layout_faults(surface);
    assert_no_text_overlap(surface);
    assert_prototype_chrome(surface, "WEEKLY WEATHER", "6 | 8");
    assert_has_text(surface, "5-DAY TREND");
    assert_has_text(surface, "HIGH / LOW");
    assert_has_text(surface, "TODAY");
    TEST_ASSERT_NULL(find_text(surface, "YESTERDAY"));
    assert_has_text(surface, "JUL 21");
    assert_has_text(surface, "JUL 22");
    assert_has_text(surface, "68/57");
    TEST_ASSERT_NULL(find_text(surface, "Rain chances stay light through the work week."));
}

void test_weather_pages_convert_celsius_snapshot_to_fahrenheit_display() {
    WeatherPageSnapshot snapshot = sampleWeatherPageSnapshot();
    snapshot.tempUnit = "F";
    snapshot.currentTempC = 22;
    snapshot.feelsLikeC = 21;
    snapshot.indoorTempC = 23;
    snapshot.weekly = {
        {"TUE", 3, 20, 15, "JUL 21"},
        {"WED", 61, 20, 14, "JUL 22"},
        {"THU", 1, 23, 16, "JUL 23"},
        {"FRI", 1, 24, 17, "JUL 24"},
        {"SAT", 3, 22, 14, "JUL 25"},
    };

    MemoryDrawSurface today(400, 300);
    renderWeatherTodayPage(today, snapshot);
    assert_has_text(today, "72F");
    assert_has_text(today, "68/57");
    TEST_ASSERT_NULL(find_text(today, "22F"));

    MemoryDrawSurface weekly(400, 300);
    renderWeeklyWeatherPage(weekly, snapshot);
    assert_has_text(weekly, "68/57");

    MemoryDrawSurface indoor(400, 300);
    renderIndoorClimatePage(indoor, snapshot);
    assert_has_text(indoor, "73F");
    assert_has_text(indoor, "70F");
}

void test_weather_pages_keep_celsius_display_when_unit_is_celsius() {
    WeatherPageSnapshot snapshot = sampleWeatherPageSnapshot();
    snapshot.tempUnit = "C";
    snapshot.currentTempC = 22;
    snapshot.feelsLikeC = 21;
    snapshot.indoorTempC = 23;
    snapshot.weekly = {
        {"TUE", 3, 20, 15, "JUL 21"},
        {"WED", 61, 20, 14, "JUL 22"},
        {"THU", 1, 23, 16, "JUL 23"},
        {"FRI", 1, 24, 17, "JUL 24"},
        {"SAT", 3, 22, 14, "JUL 25"},
    };

    MemoryDrawSurface today(400, 300);
    renderWeatherTodayPage(today, snapshot);
    assert_has_text(today, "22C");
    assert_has_text(today, "20/14");
    TEST_ASSERT_NULL(find_text(today, "72F"));

    MemoryDrawSurface weekly(400, 300);
    renderWeeklyWeatherPage(weekly, snapshot);
    assert_has_text(weekly, "20/14");

    MemoryDrawSurface indoor(400, 300);
    renderIndoorClimatePage(indoor, snapshot);
    assert_has_text(indoor, "23C");
    assert_has_text(indoor, "21C");
}

void test_weather_today_hourly_chart_uses_three_hour_24h_labels() {
    MemoryDrawSurface surface(400, 300);

    renderWeatherTodayPage(surface, sampleWeatherPageSnapshot());

    assert_no_layout_faults(surface);
    assert_has_text(surface, "00");
    assert_has_text(surface, "03");
    assert_has_text(surface, "06");
    assert_has_text(surface, "09");
    assert_has_text(surface, "12");
    assert_has_text(surface, "15");
    assert_has_text(surface, "18");
    assert_has_text(surface, "21");
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "00"));
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "03"));
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "06"));
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "09"));
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "12"));
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "15"));
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "18"));
    TEST_ASSERT_EQUAL_UINT32(1, count_text(surface, "21"));
    const auto *midnight = find_text(surface, "00");
    TEST_ASSERT_NOT_NULL(midnight);
    TEST_ASSERT_EQUAL_INT16(214, midnight->y);
}

void test_weather_today_hourly_chart_scales_celsius_temperatures_into_view() {
    WeatherPageSnapshot snapshot = sampleWeatherPageSnapshot();
    snapshot.tempUnit = "C";
    snapshot.currentTempC = 30;
    snapshot.hourly = {
        {"00", 28, 4},
        {"03", 27, 16},
        {"06", 25, 46},
        {"09", 29, 30},
        {"12", 32, 0},
        {"15", 33, 0},
        {"18", 32, 0},
        {"21", 30, 8},
    };
    ColorProbeSurface surface(400, 300);

    renderWeatherTodayPage(surface, snapshot);

    TEST_ASSERT_GREATER_THAN_UINT32(
        0, surface.countColorInRect(kDashboardAccent, Rect{168, 82, 180, 116}));
}

void test_world_clock_keeps_title_left_and_separates_card_text() {
    const WorldClockPageSnapshot snapshot = sampleWorldClockPageSnapshot();
    MemoryDrawSurface surface(400, 300);

    renderWorldClockPage(surface, snapshot);

    const auto *pageTitle = find_text(surface, "WORLD CLOCK");
    const auto *dateMeta = find_text(surface, "MON JUL 20, 2026");
    const auto *cityTitle = find_text(surface, "NEW YORK");
    const auto *timeText = find_text(surface, "08:42");
    const auto *zoneStatus = find_text(surface, "UTC-04 - MORNING");
    TEST_ASSERT_NOT_NULL(pageTitle);
    TEST_ASSERT_NOT_NULL(dateMeta);
    TEST_ASSERT_NOT_NULL(cityTitle);
    TEST_ASSERT_NOT_NULL(timeText);
    TEST_ASSERT_NOT_NULL(zoneStatus);
    TEST_ASSERT_NULL(find_text(surface, "EDT - WORKING"));
    TEST_ASSERT_EQUAL_UINT(1, count_text(surface, "WORLD CLOCK"));
    TEST_ASSERT_TRUE(has_top_right_text(surface));
    assert_prototype_chrome(surface, "WORLD CLOCK", "4 | 8", "MON JUL 20, 2026");
    TEST_ASSERT_EQUAL_INT16(382, dateMeta->x);
    TEST_ASSERT_EQUAL_INT16(34, dateMeta->y);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TextAlign::Right),
                            static_cast<uint8_t>(dateMeta->align));
    TEST_ASSERT_EQUAL_INT16(42, pageTitle->x);
    TEST_ASSERT_EQUAL_INT16(18, pageTitle->y);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TextAlign::Left),
                            static_cast<uint8_t>(pageTitle->align));
    TEST_ASSERT_EQUAL_INT16(26, cityTitle->x);
    TEST_ASSERT_EQUAL_INT16(78, cityTitle->y);
    TEST_ASSERT_EQUAL_INT16(108, timeText->y);
    TEST_ASSERT_EQUAL_INT16(132, zoneStatus->y);

    WorldClockPageSnapshot duplicateMetadata = snapshot;
    duplicateMetadata.subtitle = duplicateMetadata.title;
    MemoryDrawSurface duplicateSurface(400, 300);
    renderWorldClockPage(duplicateSurface, duplicateMetadata);
    TEST_ASSERT_NULL(find_text(duplicateSurface, "WORLD CLOCK   4/12"));
    assert_has_text(duplicateSurface, "4 | 8");
}

void test_focus_clock_page_uses_dedicated_countdown_layout() {
    FocusClockConfig cfg;
    cfg.focusMinutes = 25;
    cfg.breakMinutes = 5;
    cfg.sessionCount = 4;
    const FocusClockRuntimeState state = startFocusClockSession(cfg, 1784551320LL);
    const FocusClockPageSnapshot snapshot = focusClockPageSnapshotAt(
        1784551320LL, "Asia/Shanghai", 10, 16, cfg, state);
    MemoryDrawSurface surface(400, 300);

    renderFocusClockPage(surface, snapshot, 10, 16);

    assert_no_layout_faults(surface);
    assert_no_text_overlap(surface);
    assert_prototype_chrome(surface, "FOCUS CLOCK", "10 | 16",
                            "MON 09:42 JUL 20, 2026");
    assert_has_text(surface, "25 MIN");
    assert_has_text(surface, "IN FOCUS");
    assert_has_text(surface, "CYCLE 1 OF 4");
    assert_has_text(surface, "25 MIN FOCUS");
    assert_has_text(surface, "5 MIN BREAK");
    assert_has_text(surface, "NEXT BREAK");
    assert_has_text(surface, "END TIME");
    assert_has_text(surface, "USER HOLD 2S START / STOP");
    TEST_ASSERT_NULL(find_text(surface, "NEW YORK"));
    TEST_ASSERT_NULL(find_text(surface, "LONDON"));
    TEST_ASSERT_NULL(find_text(surface, "UTC-04 - MORNING"));
}

void test_weather_time_and_news_pages_stay_inside_400x300() {
    MemoryDrawSurface surface(400, 300);

    renderWeatherTodayPage(surface, sampleWeatherPageSnapshot());
    renderWeeklyWeatherPage(surface, sampleWeatherPageSnapshot());
    renderIndoorClimatePage(surface, sampleWeatherPageSnapshot());
    renderWorldClockPage(surface, sampleWorldClockPageSnapshot());
    renderFocusClockPage(surface, sampleFocusClockPageSnapshot());
    renderHeadlinesPage(surface, sampleNewsPageSnapshot());
    renderTodayInHistoryPage(surface, sampleNewsPageSnapshot());

    assert_no_layout_faults(surface);
}

void test_finance_pages_match_prototype_text_and_stay_inside_400x300() {
    const FinancePageSnapshot snapshot = sampleFinancePageSnapshot();

    {
        MemoryDrawSurface surface(400, 300);
        renderStockInfoPage(surface, snapshot);
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "STOCK INFO", "7 | 8");
        assert_has_text(surface, "STOCK INFO");
        assert_has_text(surface, "Delayed");
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderPortfolioSummaryPage(surface, snapshot);
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "PORTFOLIO", "7 | 8");
        assert_has_text(surface, "PORTFOLIO SUMMARY");
        assert_has_text(surface, "Not investment advice");
    }
    {
        MemoryDrawSurface surface(400, 300);
        renderEconomicCalendarPage(surface, snapshot);
        assert_no_layout_faults(surface);
        assert_prototype_chrome(surface, "ECONOMIC CALENDAR", "8 | 8");
        assert_has_text(surface, "ECONOMIC CALENDAR");
        assert_has_text(surface, "Source");
        assert_has_text(surface, "Updated");
    }
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_calendar_pages_stay_inside_400x300);
    RUN_TEST(test_today_overview_matches_compact_prototype_layout);
    RUN_TEST(test_prototype_chrome_accepts_runtime_time_battery_and_ip);
    RUN_TEST(test_monthly_overview_uses_event_list_layout);
    RUN_TEST(test_weekly_timeline_has_clear_header_and_roomy_event_cards);
    RUN_TEST(test_today_agenda_uses_ascii_text_and_clear_status);
    RUN_TEST(test_prototype_wifi_icon_is_compact_and_balanced);
    RUN_TEST(test_epd_pages_do_not_overlap_text_or_header);
    RUN_TEST(test_pages_reset_font_state_between_renderers);
    RUN_TEST(test_weather_time_and_news_pages_match_prototype_text);
    RUN_TEST(test_weekly_weather_uses_five_day_trend_layout);
    RUN_TEST(test_weather_today_hourly_chart_uses_three_hour_24h_labels);
    RUN_TEST(test_weather_pages_convert_celsius_snapshot_to_fahrenheit_display);
    RUN_TEST(test_weather_pages_keep_celsius_display_when_unit_is_celsius);
    RUN_TEST(test_weather_today_hourly_chart_scales_celsius_temperatures_into_view);
    RUN_TEST(test_world_clock_keeps_title_left_and_separates_card_text);
    RUN_TEST(test_focus_clock_page_uses_dedicated_countdown_layout);
    RUN_TEST(test_weather_time_and_news_pages_stay_inside_400x300);
    RUN_TEST(test_finance_pages_match_prototype_text_and_stay_inside_400x300);
    UNITY_END();
}

void loop() {}
