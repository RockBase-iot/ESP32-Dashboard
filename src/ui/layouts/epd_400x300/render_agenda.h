#pragma once

#include <stddef.h>

#include "ui/layouts/epd_400x300/render_overview.h"

void renderTodayAgendaPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                           size_t pageNumber = 2, size_t pageCount = 8);
void renderLocalNotesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                          size_t pageNumber = 6, size_t pageCount = 8);
void renderImportantMilestonesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                                   size_t pageNumber = 8, size_t pageCount = 8);
