#pragma once

#include <stddef.h>

#include <string>
#include <vector>

#include "app/time/focus_clock_model.h"
#include "app/time/world_clock_model.h"
#include "ui/components/calm_grid.h"
#include "ui/canvas/draw_surface.h"

struct WorldClockPageSnapshot {
    std::string title;
    std::string subtitle;
    std::string pageIndicator;
    WorldClockModel model;
};

WorldClockPageSnapshot sampleWorldClockPageSnapshot();
FocusClockPageSnapshot sampleFocusClockPageSnapshot();
WorldClockPageSnapshot worldClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount);
WorldClockPageSnapshot worldClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount,
                                                const WorldClockConfig &config);
void renderWorldClockPage(IDrawSurface &surface, const WorldClockPageSnapshot &snapshot,
                          size_t pageNumber = 4, size_t pageCount = 8,
                          const std::string &ipText = "IP: --",
                          calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderFocusClockPage(IDrawSurface &surface, const FocusClockPageSnapshot &snapshot,
                          size_t pageNumber = 4, size_t pageCount = 8,
                          const std::string &ipText = "IP: --",
                          calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
