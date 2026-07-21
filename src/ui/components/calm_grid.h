#pragma once

#include <string>
#include <vector>

#include "ui/canvas/draw_surface.h"

namespace calm_grid {

struct LabelValue {
    std::string label;
    std::string value;
    std::string detail;
    bool accent = false;
};

std::string fitText(IDrawSurface &surface, const std::string &text,
                    int16_t maxWidth, uint8_t size = 1);
void drawPageHeader(IDrawSurface &surface, const std::string &title,
                    const std::string &rightText,
                    const std::string &statusText = "",
                    int16_t accentWidth = 74);
void drawHeader(IDrawSurface &surface, const std::string &title,
                const std::string &subtitle, const std::string &rightText);
void drawPanel(IDrawSurface &surface, const Rect &rect, const std::string &title,
               const std::vector<LabelValue> &items);
void drawList(IDrawSurface &surface, const Rect &rect, const std::string &title,
              const std::vector<std::string> &items, bool accentTitle = false);
void drawMetricRow(IDrawSurface &surface, const Rect &rect, const std::vector<LabelValue> &items);
void drawMetricGrid(IDrawSurface &surface, const Rect &rect, const std::vector<LabelValue> &items,
                    uint8_t columns = 2);
void drawSectionMarker(IDrawSurface &surface, int16_t x, int16_t y,
                       uint16_t color = kDashboardAccent);
void drawMonthGrid(IDrawSurface &surface, const Rect &rect,
                   const std::vector<std::string> &cells,
                   const std::string &monthLabel);
void drawTimeline(IDrawSurface &surface, const Rect &rect,
                  const std::vector<LabelValue> &items);

}  // namespace calm_grid
