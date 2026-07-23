#include "render_calendar.h"

#include <array>
#include <utility>

#include "ui/components/calm_grid.h"

namespace {
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

void drawMonthEventCard(IDrawSurface &surface, const Rect &rect, const char *day,
                        const char *dow, const char *time, const char *title,
                        const char *detail, bool accent) {
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
                               size_t pageNumber, size_t pageCount) {
    (void)snapshot;
    calm_grid::drawPrototypePageChrome(surface, "MONTHLY OVERVIEW",
                                       calm_grid::PageIconKind::Month, pageNumber, pageCount);

    const int16_t gridX = 18;
    const int16_t cellW = 52;
    // surface.drawText(18, 66, "JULY 2026", kDashboardBlack, TextAlign::Left, 2);
    surface.drawText(18, 60, "JULY 2026", kDashboardBlack, TextAlign::Left, 2);
    drawCalendarMonthWeekHeader(surface, gridX, 86, cellW);
    // TODO: change to dynamic dates based on the real calendar date; the active day should be highlighted in accent color
    const std::array<const char *, 7> dates = {{"20", "21", "22", "23", "24", "25", "26"}};
    for (size_t i = 0; i < dates.size(); ++i) {
        const int16_t cx = static_cast<int16_t>(gridX + static_cast<int16_t>(i) * cellW + cellW / 2);
        const bool active = i == 2;
        surface.drawText(cx, 100, dates[i], active ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Center, 1);
    }
    surface.drawLine(gridX, 110, static_cast<int16_t>(gridX + cellW * 7), 116, kDashboardBlack);
    // TODO: change to dynamic events based on the real calendar date; the active day should be highlighted in accent color
    drawMonthEventCard(surface, Rect{18, 120, 176, 30}, "04", "SAT", "09:00",
                       "Holiday", "Family calendar", false);
    drawMonthEventCard(surface, Rect{206, 120, 176, 30}, "08", "WED", "10:30",
                       "Dentist", "Downtown clinic", false);
    drawMonthEventCard(surface, Rect{18, 158, 176, 30}, "10", "FRI", "14:00",
                       "Q3 Rev", "Finance deck", false);
    drawMonthEventCard(surface, Rect{206, 158, 176, 30}, "17", "FRI", "17:30",
                       "Deadline", "Prototype freeze", false);
    drawMonthEventCard(surface, Rect{18, 196, 176, 30}, "20", "MON", "09:00",
                       "Review", "Task 14 UI", false);
    drawMonthEventCard(surface, Rect{206, 196, 176, 30}, "22", "WED", "18:30",
                       "Birthday", "Dinner reservation", true);
    drawMonthEventCard(surface, Rect{18, 234, 176, 30}, "25", "SAT", "16:00",
                       "Soccer", "Community field", false);
    drawMonthEventCard(surface, Rect{206, 234, 176, 30}, "28", "TUE", "11:00",
                       "Atlas", "Project sync", false);
}

void renderWeeklyTimelinePage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                              size_t pageNumber, size_t pageCount) {
    calm_grid::drawPrototypePageChrome(surface, "WEEKLY TIMELINE",
                                       calm_grid::PageIconKind::Week, pageNumber, pageCount);

    const int16_t gridX = 18;
    const int16_t cellW = 52;
    const int16_t gridBottom = 256;
    const int activeDayIndex = 2;

    // surface.drawText(18, 66, "JUL 20-26 2026", kDashboardBlack, TextAlign::Left, 1);
    // drawCalendarWeekHeader(surface, gridX, 92, cellW, activeDayIndex);
    drawCalendarWeekHeader(surface, gridX, 62, cellW, activeDayIndex);
    for (int i = 0; i <= 7; ++i) {
        const int16_t x = static_cast<int16_t>(gridX + i * cellW);
        surface.drawLine(x, 88, x, gridBottom, kDashboardBlack);
    }
    surface.drawLine(gridX, 88, static_cast<int16_t>(gridX + 7 * cellW), 88, kDashboardBlack);

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
        const int16_t y = static_cast<int16_t>(98 + slot * 58);
        const Rect box{static_cast<int16_t>(x + 4), y, 44, 48};
        drawTimelineEventCard(surface, box, item);
    }
    // TODO: change this to a dynamic calculation based on the number of events in the timeline
    surface.drawText(18, 270, "NEXT 09:00 - Design review", kDashboardAccent, TextAlign::Left, 1);
}
