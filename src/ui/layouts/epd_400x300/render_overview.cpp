#include "render_overview.h"

#include <array>
#include <utility>

#include "ui/components/calm_grid.h"

namespace {
constexpr int16_t kOverviewMargin = 18;

void drawOverviewHeader(IDrawSurface &surface, const std::string &title, const std::string &rightText) {
    calm_grid::drawPageHeader(surface, title, "", rightText, 74);
}

void drawOverviewFooter(IDrawSurface &surface, const std::string &leftText, const std::string &rightText) {
    surface.drawText(kOverviewMargin, 286, calm_grid::fitText(surface, leftText, 166, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(392, 286, calm_grid::fitText(surface, rightText, 206, 1),
                     kDashboardBlack, TextAlign::Right, 1);
}

void drawOverviewWeekStrip(IDrawSurface &surface, const Rect &rect) {
    const std::array<const char *, 7> dayLabels = {"M", "T", "W", "T", "F", "S", "S"};
    const std::array<const char *, 7> dayDates = {"20", "21", "22", "23", "24", "25", "26"};
    const int16_t cellW = 50;
    for (size_t i = 0; i < dayLabels.size(); ++i) {
        const int16_t x = static_cast<int16_t>(rect.x + static_cast<int16_t>(i) * 52);
        const bool active = i == 0;
        if (active) {
            surface.fillRect(x, rect.y, cellW, rect.h, kDashboardAccent);
        } else {
            surface.fillRect(x, rect.y, cellW, rect.h, kDashboardWhite);
        }
        surface.drawRect(x, rect.y, cellW, rect.h, active ? kDashboardAccent : kDashboardBlack);
        surface.drawText(static_cast<int16_t>(x + cellW / 2), static_cast<int16_t>(rect.y + 10),
                         dayLabels[i], active ? kDashboardWhite : kDashboardBlack,
                         TextAlign::Center, 1);
        surface.drawText(static_cast<int16_t>(x + cellW / 2), static_cast<int16_t>(rect.y + 20),
                         dayDates[i], active ? kDashboardWhite : kDashboardBlack,
                         TextAlign::Center, 1);
    }
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
    surface.drawText(static_cast<int16_t>(rect.x + 5), static_cast<int16_t>(rect.y + 25),
                     calm_grid::fitText(surface, line1, static_cast<int16_t>(rect.w - 10), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    if (!line2.empty()) {
        surface.drawText(static_cast<int16_t>(rect.x + 5), static_cast<int16_t>(rect.y + 37),
                         calm_grid::fitText(surface, line2, static_cast<int16_t>(rect.w - 10), 1),
                         kDashboardBlack, TextAlign::Left, 1);
    }
}
}  // namespace

CalendarPageSnapshot sampleCalendarPageSnapshot() {
    CalendarPageSnapshot snapshot;
    snapshot.title = "TODAY OVERVIEW";
    snapshot.subtitle = "WiFi:192.168.1.42 BAT:82%";
    snapshot.overviewItems = {
        {"09:00", "Design review", "Office - Project Atlas", true},
        {"15:00", "Pick up Mia", "School entrance", false},
        {"18:30", "Family dinner", "Home", false},
    };
    snapshot.timelineItems = {
        {"09:00", "Review", "Project Atlas", true, 0},
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
        if (i == 21) {
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

void renderOverviewPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot) {
    drawOverviewHeader(surface, snapshot.title, snapshot.subtitle);
    drawOverviewWeekStrip(surface, Rect{18, 54, 364, 24});

    const Rect agenda{18, 86, 280, 114};
    const int16_t rowH = 38;
    for (size_t i = 0; i < snapshot.overviewItems.size() && i < 3; ++i) {
        drawOverviewAgendaRow(surface,
                              Rect{agenda.x, static_cast<int16_t>(agenda.y + rowH * static_cast<int16_t>(i)),
                                   agenda.w, static_cast<int16_t>(rowH - 1)},
                              snapshot.overviewItems[i]);
    }

    drawOverviewSidebarCard(surface, Rect{306, 86, 76, 42}, "LOCAL NOTE",
                            snapshot.notes.size() > 0 ? snapshot.notes[0] : "",
                            snapshot.notes.size() > 1 ? snapshot.notes[1] : "");
    drawOverviewSidebarCard(surface, Rect{306, 138, 76, 44}, "72F", "Sunny", "Rain 10%", true);

    drawOverviewFooter(surface, "Updated 08:42", "BOOT next - USER previous");
}
