#include "render_agenda.h"

#include "ui/components/calm_grid.h"

void renderTodayAgendaPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                           size_t pageNumber, size_t pageCount) {
    calm_grid::drawPrototypePageChrome(surface, "TODAY AGENDA",
                                       calm_grid::PageIconKind::Overview, pageNumber, pageCount);
    calm_grid::drawList(surface, Rect{18, 60, 364, 135}, "Today", snapshot.agendaItems, true);
    calm_grid::drawList(surface, Rect{18, 203, 170, 75}, "Notes", snapshot.notes, false);
    calm_grid::drawList(surface, Rect{206, 203, 176, 75}, "Milestones", snapshot.milestones, false);
}

void renderLocalNotesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                          size_t pageNumber, size_t pageCount) {
    calm_grid::drawPrototypePageChrome(surface, "LOCAL NOTES",
                                       calm_grid::PageIconKind::Overview, pageNumber, pageCount);
    calm_grid::drawList(surface, Rect{18, 60, 364, 212}, "Notes", snapshot.notes, true);
}

void renderImportantMilestonesPage(IDrawSurface &surface, const CalendarPageSnapshot &snapshot,
                                   size_t pageNumber, size_t pageCount) {
    calm_grid::drawPrototypePageChrome(surface, "IMPORTANT MILESTONES",
                                       calm_grid::PageIconKind::Month, pageNumber, pageCount);
    calm_grid::drawList(surface, Rect{18, 60, 364, 212}, "Important Milestones",
                        snapshot.milestones, true);
}
