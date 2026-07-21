#pragma once

#include "ui/layouts/epd_400x300/render_overview.h"

void renderTodayAgendaPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot);
void renderLocalNotesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot);
void renderImportantMilestonesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot);
