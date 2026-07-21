#pragma once

#include "ui/layouts/epd_400x300/render_overview.h"

void renderMonthlyOverviewPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot);
void renderWeeklyTimelinePage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot);
