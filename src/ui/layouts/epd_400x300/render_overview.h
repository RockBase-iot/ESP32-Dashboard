#pragma once

#include <stddef.h>

#include <array>
#include <string>
#include <vector>

#include "ui/components/calm_grid.h"

struct CalendarDayCell {
    std::string text;
    std::string detail;
    bool muted = false;
    bool today = false;
    bool accent = false;
};

struct CalendarEventLine {
    std::string time;
    std::string title;
    std::string detail;
    bool accent = false;
    int dayIndex = -1;
};

struct CalendarPageSnapshot {
    std::string title;
    std::string subtitle;
    std::string dateTitle;
    std::string weekRangeLabel;
    std::vector<CalendarEventLine> overviewItems;
    std::vector<CalendarEventLine> timelineItems;
    std::vector<CalendarDayCell> weekCells;
    std::vector<CalendarDayCell> monthCells;
    std::vector<std::string> agendaItems;
    std::vector<std::string> notes;
    std::vector<std::string> milestones;
};

CalendarPageSnapshot sampleCalendarPageSnapshot();
void renderOverviewPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                        size_t pageNumber = 1, size_t pageCount = 8,
                        const std::string &ipText = "IP: --",
                        calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
