#pragma once

#include <string>
#include <vector>

#include "ui/canvas/draw_surface.h"

struct NewsItemCell {
    std::string title;
    std::string source;
    std::string detail;
    bool accent = false;
};

struct NewsPageSnapshot {
    std::string title;
    std::string subtitle;
    std::vector<NewsItemCell> headlines;
    std::vector<NewsItemCell> history;
};

NewsPageSnapshot sampleNewsPageSnapshot();
void renderHeadlinesPage(IDrawSurface &surface, const NewsPageSnapshot &snapshot);
void renderTodayInHistoryPage(IDrawSurface &surface, const NewsPageSnapshot &snapshot);
