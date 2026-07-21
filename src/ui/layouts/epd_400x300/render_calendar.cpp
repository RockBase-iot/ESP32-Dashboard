#include "render_calendar.h"

#include <array>
#include <utility>

#include "ui/components/calm_grid.h"

namespace {
void drawCalendarHeader(IDrawSurface &surface, const std::string &title, const std::string &rightText) {
    calm_grid::drawPageHeader(surface, title, rightText, "", 74);
}

void drawCalendarFooter(IDrawSurface &surface, const std::string &leftText, const std::string &rightText) {
    surface.drawText(18, 286, leftText, kDashboardAccent, TextAlign::Left, 1);
    surface.drawText(392, 286, rightText, kDashboardBlack, TextAlign::Right, 1);
}

std::string calendarMonthTitle(const CalendarDayCell &cell) {
    if (cell.detail.empty()) {
        return cell.text;
    }
    return cell.text;
}

void drawCalendarMonthCell(IDrawSurface &surface, const Rect &cell, const CalendarDayCell &day) {
    surface.drawRect(cell.x, cell.y, cell.w, cell.h, day.today ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(cell.x + 4), static_cast<int16_t>(cell.y + 11),
                     calendarMonthTitle(day), day.today || day.accent ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 1);
    if (!day.detail.empty()) {
        surface.drawText(static_cast<int16_t>(cell.x + 4), static_cast<int16_t>(cell.y + 27),
                         calm_grid::fitText(surface, day.detail, static_cast<int16_t>(cell.w - 8), 1),
                         kDashboardBlack, TextAlign::Left, 1);
    }
}

std::array<std::pair<const char *, int>, 7> calendarWeeklyLabels() {
    return {{{"MON", 20}, {"TUE", 21}, {"WED", 22}, {"THU", 23}, {"FRI", 24}, {"SAT", 25}, {"SUN", 26}}};
}

void drawCalendarMonthWeekHeader(IDrawSurface &surface, int16_t x, int16_t y, int16_t cellW) {
    const std::array<const char *, 7> labels = {{"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"}};
    for (size_t i = 0; i < labels.size(); ++i) {
        const int16_t cx = static_cast<int16_t>(x + static_cast<int16_t>(i) * cellW);
        const bool weekend = i >= 5;
        surface.drawText(static_cast<int16_t>(cx + cellW / 2), y, labels[i],
                         weekend ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
    }
}

void drawCalendarWeekHeader(IDrawSurface &surface, int16_t x, int16_t y, int16_t cellW,
                            int activeDayIndex) {
    const auto labels = calendarWeeklyLabels();
    for (size_t i = 0; i < labels.size(); ++i) {
        const int16_t cx = static_cast<int16_t>(x + static_cast<int16_t>(i) * cellW);
        const bool active = static_cast<int>(i) == activeDayIndex;
        surface.drawText(static_cast<int16_t>(cx + cellW / 2), y, labels[i].first,
                         active ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
        surface.drawText(static_cast<int16_t>(cx + cellW / 2), static_cast<int16_t>(y + 13),
                         std::to_string(labels[i].second),
                         active ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
    }
}

void drawTimelineEventCard(IDrawSurface &surface, const Rect &rect, const CalendarEventLine &item) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, item.accent ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 11),
                     item.time, item.accent ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 1);
    surface.drawLine(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 15),
                     static_cast<int16_t>(rect.x + rect.w - 4), static_cast<int16_t>(rect.y + 15),
                     item.accent ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 24),
                     calm_grid::fitText(surface, item.title, static_cast<int16_t>(rect.w - 8), 1),
                     kDashboardBlack, TextAlign::Left, 1);
}
}  // namespace

void renderMonthlyOverviewPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot) {
    (void)snapshot;
    drawCalendarHeader(surface, "JULY 2026", "2/12");

    const int16_t gridX = 18;
    const int16_t gridY = 74;
    const int16_t cellW = 52;
    const int16_t cellH = 40;

    drawCalendarMonthWeekHeader(surface, gridX, 61, cellW);
    surface.drawLine(gridX, 70, static_cast<int16_t>(gridX + cellW * 7), 70, kDashboardBlack);

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 7; ++col) {
            const int idx = row * 7 + col;
            if (idx >= static_cast<int>(snapshot.monthCells.size())) {
                continue;
            }
            const Rect cell{static_cast<int16_t>(gridX + col * cellW),
                            static_cast<int16_t>(gridY + row * cellH),
                            cellW, cellH};
            drawCalendarMonthCell(surface, cell, snapshot.monthCells[idx]);
        }
    }
}

void renderWeeklyTimelinePage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot) {
    drawCalendarHeader(surface, "WEEKLY TIMELINE", "JUL 20-26 2026   3/12");

    const int16_t gridX = 18;
    const int16_t cellW = 52;
    const int16_t gridBottom = 256;
    const int activeDayIndex = 0;

    drawCalendarWeekHeader(surface, gridX, 58, cellW, activeDayIndex);
    for (int i = 0; i <= 7; ++i) {
        const int16_t x = static_cast<int16_t>(gridX + i * cellW);
        surface.drawLine(x, 82, x, gridBottom, kDashboardBlack);
    }
    surface.drawLine(gridX, 82, static_cast<int16_t>(gridX + 7 * cellW), 82, kDashboardBlack);

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
        const int16_t y = static_cast<int16_t>(90 + slot * 36);
        const Rect box{static_cast<int16_t>(x + 3), y, 46, 30};
        drawTimelineEventCard(surface, box, item);
    }

    drawCalendarFooter(surface, "NEXT 09:00 - Design review", "Updated 08:42");
}
