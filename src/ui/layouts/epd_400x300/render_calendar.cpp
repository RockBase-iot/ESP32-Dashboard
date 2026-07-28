#include "render_calendar.h"

#include <array>
#include <utility>

#include "ui/components/calm_grid.h"

namespace {
std::vector<CalendarDayCell> fallbackWeekCells() {
    const std::array<const char *, 7> labels = {{"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"}};
    const std::array<const char *, 7> dates = {{"20", "21", "22", "23", "24", "25", "26"}};
    std::vector<CalendarDayCell> cells;
    cells.reserve(7);
    for (size_t i = 0; i < labels.size(); ++i) {
        cells.push_back({dates[i], labels[i], false, i == 2, i == 2});
    }
    return cells;
}

std::vector<CalendarDayCell> weekCellsForRender(const CalendarPageSnapshot &snapshot) {
    return snapshot.weekCells.empty() ? fallbackWeekCells() : snapshot.weekCells;
}

void drawCalendarMonthWeekHeader(IDrawSurface &surface, int16_t x, int16_t y, int16_t cellW,
                                 const std::vector<CalendarDayCell> &cells) {
    for (size_t i = 0; i < cells.size() && i < 7; ++i) {
        const int16_t cx = static_cast<int16_t>(x + static_cast<int16_t>(i) * cellW);
        const bool weekend = i >= 5;
        const bool active = cells[i].today || cells[i].accent;
        surface.drawText(static_cast<int16_t>(cx + cellW / 2), y, cells[i].detail,
                         active || weekend ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Center, 1);
    }
}

void drawCalendarWeekHeader(IDrawSurface &surface, int16_t x, int16_t y, int16_t cellW,
                            const std::vector<CalendarDayCell> &cells) {
    for (size_t i = 0; i < cells.size() && i < 7; ++i) {
        const int16_t cx = static_cast<int16_t>(x + static_cast<int16_t>(i) * cellW);
        const bool active = cells[i].today || cells[i].accent;
        surface.drawText(static_cast<int16_t>(cx + cellW / 2), y, cells[i].detail,
                         active ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
        surface.drawText(static_cast<int16_t>(cx + cellW / 2), static_cast<int16_t>(y + 13),
                         cells[i].text,
                         active ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
    }
}

void drawTimelineEventCard(IDrawSurface &surface, const Rect &rect, const CalendarEventLine &item) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, item.accent ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 6),
                     item.time, item.accent ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 1);
    surface.drawLine(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 15),
                     static_cast<int16_t>(rect.x + rect.w - 4), static_cast<int16_t>(rect.y + 15),
                     item.accent ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 24),
                     calm_grid::fitText(surface, item.title, static_cast<int16_t>(rect.w - 8), 1),
                     kDashboardBlack, TextAlign::Left, 1);
}

void drawMonthEventCard(IDrawSurface &surface, const Rect &rect, const std::string &day,
                        const std::string &dow, const std::string &time, const std::string &title,
                        const std::string &detail, bool accent) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, accent ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 10), static_cast<int16_t>(rect.y + 6),
                     day, accent ? kDashboardAccent : kDashboardBlack, TextAlign::Left, 2);
    surface.drawText(static_cast<int16_t>(rect.x + 48), static_cast<int16_t>(rect.y + 6),
                     dow, kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 48), static_cast<int16_t>(rect.y + 18),
                     time, kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 93), static_cast<int16_t>(rect.y + 6),
                     calm_grid::fitText(surface, title, static_cast<int16_t>(rect.w - 95), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 93), static_cast<int16_t>(rect.y + 18),
                     calm_grid::fitText(surface, detail, static_cast<int16_t>(rect.w - 95), 1),
                     kDashboardBlack, TextAlign::Left, 1);
}
}  // namespace

void renderMonthlyOverviewPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                               size_t pageNumber, size_t pageCount,
                               const std::string &ipText, calm_grid::ChromeContext chrome) {
    calm_grid::drawPrototypePageChrome(surface, "MONTHLY OVERVIEW",
                                       calm_grid::PageIconKind::Month, pageNumber, pageCount,
                                       chrome.timeText, ipText.empty() ? chrome.ipText : ipText,
                                       chrome.batteryText);

    const int16_t gridX = 18;
    const int16_t cellW = 52;
    const std::vector<CalendarDayCell> cells = weekCellsForRender(snapshot);
    surface.drawText(18, 60, snapshot.dateTitle.empty() ? "JULY 2026" : snapshot.dateTitle,
                     kDashboardBlack, TextAlign::Left, 2);
    drawCalendarMonthWeekHeader(surface, gridX, 86, cellW, cells);
    for (size_t i = 0; i < cells.size() && i < 7; ++i) {
        const int16_t cx = static_cast<int16_t>(gridX + static_cast<int16_t>(i) * cellW + cellW / 2);
        const bool active = cells[i].today || cells[i].accent;
        surface.drawText(cx, 100, cells[i].text, active ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Center, 1);
    }
    surface.drawLine(gridX, 110, static_cast<int16_t>(gridX + cellW * 7), 110, kDashboardBlack);

    const size_t count = std::min<size_t>(8, snapshot.timelineItems.size());
    for (size_t i = 0; i < count; ++i) {
        const CalendarEventLine &item = snapshot.timelineItems[i];
        const int16_t x = static_cast<int16_t>(18 + (i % 2) * 188);
        const int16_t y = static_cast<int16_t>(120 + (i / 2) * 38);
        std::string day = "--";
        std::string dow = "---";
        if (item.dayIndex >= 0 && item.dayIndex < static_cast<int>(cells.size())) {
            day = cells[static_cast<size_t>(item.dayIndex)].text;
            dow = cells[static_cast<size_t>(item.dayIndex)].detail;
        }
        drawMonthEventCard(surface, Rect{x, y, 176, 30}, day, dow, item.time,
                           item.title, item.detail, item.accent);
    }
}

void renderWeeklyTimelinePage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                              size_t pageNumber, size_t pageCount,
                              const std::string &ipText, calm_grid::ChromeContext chrome) {
    calm_grid::drawPrototypePageChrome(surface, "WEEKLY TIMELINE",
                                       calm_grid::PageIconKind::Week, pageNumber, pageCount,
                                       chrome.timeText, ipText.empty() ? chrome.ipText : ipText,
                                       chrome.batteryText);

    const int16_t gridX = 18;
    const int16_t cellW = 52;
    const int16_t gridBottom = 256;
    const std::vector<CalendarDayCell> cells = weekCellsForRender(snapshot);

    surface.drawText(18, 58, snapshot.weekRangeLabel.empty() ? "JUL 20-26 2026" : snapshot.weekRangeLabel,
                     kDashboardBlack, TextAlign::Left, 1);
    drawCalendarWeekHeader(surface, gridX, 76, cellW, cells);
    for (int i = 0; i <= 7; ++i) {
        const int16_t x = static_cast<int16_t>(gridX + i * cellW);
        surface.drawLine(x, 104, x, gridBottom, kDashboardBlack);
    }
    surface.drawLine(gridX, 104, static_cast<int16_t>(gridX + 7 * cellW), 104, kDashboardBlack);

    std::array<int, 7> slotCounts{};
    for (const auto &item : snapshot.timelineItems) {
        if (item.dayIndex < 0 || item.dayIndex >= 7) {
            continue;
        }
        const int slot = slotCounts[static_cast<size_t>(item.dayIndex)]++;
        if (slot >= 2) {
            continue;
        }
        const int16_t x = static_cast<int16_t>(gridX + item.dayIndex * cellW);
        const int16_t y = static_cast<int16_t>(112 + slot * 56);
        const Rect box{static_cast<int16_t>(x + 4), y, 44, 48};
        drawTimelineEventCard(surface, box, item);
    }
    // TODO: change this to a dynamic calculation based on the number of events in the timeline
    surface.drawText(18, 270, "NEXT 09:00 - Design review", kDashboardAccent, TextAlign::Left, 1);
}
