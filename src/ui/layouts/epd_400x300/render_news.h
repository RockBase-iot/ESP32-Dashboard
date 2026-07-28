#pragma once

#include <stddef.h>

#include <string>
#include <vector>

#include "ui/components/calm_grid.h"

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
void renderHeadlinesPage(IDrawSurface &surface, const NewsPageSnapshot &snapshot,
                         size_t pageNumber = 5, size_t pageCount = 8,
                         const std::string &ipText = "IP: --",
                         calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderTodayInHistoryPage(IDrawSurface &surface, const NewsPageSnapshot &snapshot,
                              size_t pageNumber = 5, size_t pageCount = 8,
                              const std::string &ipText = "IP: --",
                              calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
