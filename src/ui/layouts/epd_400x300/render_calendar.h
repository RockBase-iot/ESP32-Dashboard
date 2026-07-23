#pragma once

#include <stddef.h>

#include "ui/layouts/epd_400x300/render_overview.h"

void renderMonthlyOverviewPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                               size_t pageNumber = 2, size_t pageCount = 8);
void renderWeeklyTimelinePage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                              size_t pageNumber = 3, size_t pageCount = 8);
