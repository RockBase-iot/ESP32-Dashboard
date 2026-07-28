#include "render_overview.h"

#include <array>
#include <utility>

#include "ui/components/calm_grid.h"

namespace {
constexpr int16_t kOverviewMargin = 18;

std::vector<CalendarDayCell> defaultWeekCells() {
    const std::array<const char *, 7> dayLabels = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
    const std::array<const char *, 7> dayDates = {"20", "21", "22", "23", "24", "25", "26"};
    std::vector<CalendarDayCell> cells;
    cells.reserve(7);
    for (size_t i = 0; i < dayLabels.size(); ++i) {
        cells.push_back({dayDates[i], dayLabels[i], false, i == 2, i == 2});
    }
    return cells;
}

void drawOverviewWeekStrip(IDrawSurface &surface, const Rect &rect,
                           const std::vector<CalendarDayCell> &weekCells) {
    const std::vector<CalendarDayCell> fallback = weekCells.empty() ? defaultWeekCells() : weekCells;
    const int16_t cellW = static_cast<int16_t>(rect.w / 7);
    for (size_t i = 0; i < fallback.size() && i < 7; ++i) {
        const int16_t x = static_cast<int16_t>(rect.x + static_cast<int16_t>(i) * cellW);
        const bool active = fallback[i].today || fallback[i].accent;
        surface.drawText(static_cast<int16_t>(x + cellW / 2), static_cast<int16_t>(rect.y + 10),
                         fallback[i].detail, active ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Center, 1);
        surface.drawText(static_cast<int16_t>(x + cellW / 2), static_cast<int16_t>(rect.y + 20),
                         fallback[i].text, active ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Center, 1);
    }
    surface.drawLine(rect.x, static_cast<int16_t>(rect.y + rect.h + 4),
                     static_cast<int16_t>(rect.x + rect.w), static_cast<int16_t>(rect.y + rect.h + 4),
                     kDashboardBlack);
}

void drawOverviewAgendaRow(IDrawSurface &surface, const Rect &rect, const CalendarEventLine &item) {
    surface.fillRect(rect.x, rect.y, rect.w, rect.h, kDashboardWhite);
    surface.drawLine(rect.x + 1, rect.y + 1, rect.x + 1, static_cast<int16_t>(rect.y + rect.h - 2),
                     item.accent ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + 5),
                     item.time, item.accent ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + 17),
                     calm_grid::fitText(surface, item.title, static_cast<int16_t>(rect.w - 16), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    if (!item.detail.empty()) {
        surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + 28),
                         calm_grid::fitText(surface, item.detail, static_cast<int16_t>(rect.w - 16), 1),
                         kDashboardBlack, TextAlign::Left, 1);
    }
}

void drawOverviewSidebarCard(IDrawSurface &surface, const Rect &rect, const std::string &title,
                             const std::string &line1, const std::string &line2,
                             bool accent = false) {
    surface.fillRect(rect.x, rect.y, rect.w, rect.h, kDashboardWhite);
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, accent ? kDashboardAccent : kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 5), static_cast<int16_t>(rect.y + 11),
                     calm_grid::fitText(surface, title, static_cast<int16_t>(rect.w - 10), 1),
                     accent ? kDashboardAccent : kDashboardBlack, TextAlign::Left, 1);
    surface.drawLine(static_cast<int16_t>(rect.x + 5), static_cast<int16_t>(rect.y + 20),
                     static_cast<int16_t>(rect.x + rect.w - 6), static_cast<int16_t>(rect.y + 20),
                     kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 12), static_cast<int16_t>(rect.y + 35),
                     calm_grid::fitText(surface, line1, static_cast<int16_t>(rect.w - 10), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    if (!line2.empty()) {
        surface.drawText(static_cast<int16_t>(rect.x + 12), static_cast<int16_t>(rect.y + 49),
                         calm_grid::fitText(surface, line2, static_cast<int16_t>(rect.w - 10), 1),
                         kDashboardBlack, TextAlign::Left, 1);
    }
}
}  // namespace

CalendarPageSnapshot sampleCalendarPageSnapshot() {
    CalendarPageSnapshot snapshot;
    snapshot.title = "TODAY OVERVIEW";
    snapshot.subtitle = "WiFi:on BAT:82%";
    snapshot.dateTitle = "JULY 2026";
    snapshot.weekRangeLabel = "JUL 20-26 2026";
    snapshot.weekCells = defaultWeekCells();
    snapshot.overviewItems = {
        {"09:00", "Design review", "Office - Project Atlas", true},
        {"15:00", "Pick up Mia", "School entrance", false},
        {"18:30", "Family dinner", "Home", false},
    };
    snapshot.timelineItems = {
        {"09:00", "Review", "Project Atlas", true, 2},
        {"15:00", "Mia", "School", false, 0},
        {"08:30", "Gym", "Run", false, 1},
        {"18:00", "Piano", "Lesson", false, 1},
        {"10:00", "Client", "Remote", false, 2},
        {"13:00", "Trip", "Office", false, 3},
        {"11:00", "Doctor", "Clinic", false, 4},
        {"18:30", "Dinner", "Home", false, 6},
    };
    snapshot.monthCells.resize(42);
    const std::array<int, 42> monthValues = {
        29, 30, 1, 2, 3, 4, 5,
        6, 7, 8, 9, 10, 11, 12,
        13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26,
        27, 28, 29, 30, 31, 1, 2,
        3, 4, 5, 6, 7, 8, 9,
    };
    const std::array<std::pair<int, const char *>, 8> details = {{
        {4, "Holiday"},
        {8, "Dentist"},
        {10, "Q3 Rev"},
        {17, "Deadline"},
        {20, "Review"},
        {22, "Birthday"},
        {25, "Soccer"},
        {28, "Atlas"},
    }};
    for (size_t i = 0; i < snapshot.monthCells.size(); ++i) {
        const int day = monthValues[i];
        const bool muted = (i < 2 || i > 32);
        snapshot.monthCells[i].text = std::to_string(day);
        snapshot.monthCells[i].muted = muted;
        snapshot.monthCells[i].today = (i == 21);
        if (i == 23) {
            snapshot.monthCells[i].detail = "Review";
            snapshot.monthCells[i].accent = true;
        }
        for (const auto &entry : details) {
            if (!muted && entry.first == day) {
                snapshot.monthCells[i].detail = entry.second;
                snapshot.monthCells[i].accent = true;
                break;
            }
        }
    }
    snapshot.agendaItems = {
        "09:00  User review",
        "11:15  Handoff sync",
        "14:00  Weather refresh",
        "18:30  Quiet hours",
    };
    snapshot.notes = {
        "Buy milk",
        "Call plumber",
        "No OAuth.",
    };
    snapshot.milestones = {
        "Task 8 recurrence",
        "Task 9 cache merge",
        "Task 10 config portal",
    };
    return snapshot;
}

void renderOverviewPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                        size_t pageNumber, size_t pageCount, const std::string &ipText,
                        calm_grid::ChromeContext chrome) {
    calm_grid::drawPrototypePageChrome(surface, "TODAY OVERVIEW",
                                       calm_grid::PageIconKind::Overview, pageNumber, pageCount,
                                       chrome.timeText, ipText.empty() ? chrome.ipText : ipText,
                                       chrome.batteryText);
    drawOverviewWeekStrip(surface, Rect{kOverviewMargin, 52, 364, 28}, snapshot.weekCells);

    const Rect agenda{kOverviewMargin, 86, 220, 122};
    // surface.drawRect(agenda.x, agenda.y, agenda.w, agenda.h, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(agenda.x + 8), static_cast<int16_t>(agenda.y + 16),
                     "TODAY AGENDA", kDashboardAccent, TextAlign::Left, 1);
    surface.drawLine(static_cast<int16_t>(agenda.x + 8), static_cast<int16_t>(agenda.y + 26),
                     static_cast<int16_t>(agenda.x + agenda.w - 8), static_cast<int16_t>(agenda.y + 26),
                     kDashboardBlack);
    const int16_t rowH = 38;
    for (size_t i = 0; i < snapshot.overviewItems.size() && i < 3; ++i) {
        drawOverviewAgendaRow(surface,
                              Rect{static_cast<int16_t>(agenda.x + 8),
                                   static_cast<int16_t>(agenda.y + 32 + rowH * static_cast<int16_t>(i)),
                                   static_cast<int16_t>(agenda.w - 16), static_cast<int16_t>(rowH - 4)},
                              snapshot.overviewItems[i]);
    }

    drawOverviewSidebarCard(surface, Rect{252, 86, 130, 85}, "LOCAL NOTE",
                            snapshot.notes.size() > 0 ? snapshot.notes[0] : "",
                            snapshot.notes.size() > 1 ? snapshot.notes[1] : "");
    drawOverviewSidebarCard(surface, Rect{252, 180, 130, 75}, "MILESTONES",
                            snapshot.milestones.size() > 0 ? snapshot.milestones[0] : "",
                            "Prototype refresh", true);
}
