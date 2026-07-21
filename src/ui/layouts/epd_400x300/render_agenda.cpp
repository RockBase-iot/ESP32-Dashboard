#include "render_agenda.h"

#include "ui/components/calm_grid.h"

void renderTodayAgendaPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot) {
    calm_grid::drawPageHeader(surface, "Today Agenda", "Agenda", snapshot.subtitle, 74);
    calm_grid::drawList(surface, Rect{8, 52, 384, 150}, "Today", snapshot.agendaItems, true);
    calm_grid::drawList(surface, Rect{8, 208, 180, 84}, "Notes", snapshot.notes, false);
    calm_grid::drawList(surface, Rect{204, 208, 188, 84}, "Milestones", snapshot.milestones, false);
}

void renderLocalNotesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot) {
    calm_grid::drawPageHeader(surface, "Local Notes", "Notes", snapshot.subtitle, 74);
    calm_grid::drawList(surface, Rect{8, 52, 384, 240}, "Notes", snapshot.notes, true);
}

void renderImportantMilestonesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot) {
    calm_grid::drawPageHeader(surface, "Important Milestones", "Milestones", snapshot.subtitle, 74);
    calm_grid::drawList(surface, Rect{8, 52, 384, 240}, "Milestones", snapshot.milestones, true);
}
