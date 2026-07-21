#pragma once

#include <string>
#include <vector>

#include "app/time/world_clock_model.h"
#include "ui/canvas/draw_surface.h"

struct WorldClockPageSnapshot {
    std::string title;
    std::string subtitle;
    std::string pageIndicator;
    WorldClockModel model;
};

WorldClockPageSnapshot sampleWorldClockPageSnapshot();
WorldClockPageSnapshot worldClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount);
void renderWorldClockPage(IDrawSurface &surface, const WorldClockPageSnapshot &snapshot);
void renderFocusClockPage(IDrawSurface &surface, const WorldClockPageSnapshot &snapshot);
