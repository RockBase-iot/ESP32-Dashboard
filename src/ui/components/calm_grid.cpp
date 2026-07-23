#include "calm_grid.h"

#include <algorithm>

namespace calm_grid {
namespace {
constexpr int16_t kMargin = 8;
constexpr int16_t kTitleX = 28;
constexpr int16_t kTitleY = 12;
constexpr int16_t kStatusY = 31;
constexpr int16_t kSectionGap = 8;
constexpr int16_t kHeaderRuleY = 45;
constexpr int16_t kHeaderAccentY = 46;
constexpr int16_t kRightWidth = 92;
constexpr int16_t kHeaderRightX = 382;
constexpr int16_t kHeaderRightStartX = 316;
constexpr int16_t kStatusLeftTextX = 46;
constexpr int16_t kBatteryIconX = 278;
constexpr int16_t kBatteryTextX = 296;

void box(IDrawSurface &surface, const Rect &rect, uint16_t color) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, color);
}

void fill(IDrawSurface &surface, const Rect &rect, uint16_t color) {
    surface.fillRect(rect.x, rect.y, rect.w, rect.h, color);
}

void drawHeaderIcon(IDrawSurface &surface) {
    surface.drawRect(10, 12, 12, 12, kDashboardBlack);
    surface.drawLine(12, 17, 20, 17, kDashboardBlack);
    surface.drawLine(14, 15, 14, 19, kDashboardBlack);
}

void drawWifiIcon(IDrawSurface &surface, int16_t x, int16_t y, uint16_t color) {
    surface.drawLine(x, y + 6, static_cast<int16_t>(x + 4), y, color);
    surface.drawLine(static_cast<int16_t>(x + 4), y, static_cast<int16_t>(x + 8), y + 6, color);
    surface.drawLine(static_cast<int16_t>(x + 2), static_cast<int16_t>(y + 3),
                     static_cast<int16_t>(x + 4), static_cast<int16_t>(y + 5), color);
    surface.drawPixel(static_cast<int16_t>(x + 4), static_cast<int16_t>(y + 7), color);
}

void drawBatteryIcon(IDrawSurface &surface, int16_t x, int16_t y, uint16_t color) {
    surface.drawRect(x, y + 1, 12, 7, color);
    surface.fillRect(static_cast<int16_t>(x + 12), y + 3, 2, 3, color);
}

void drawPrototypeWifiIcon(IDrawSurface &surface, int16_t x, int16_t y) {
    const auto stroke = [&surface](int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
        surface.drawLine(x0, y0, x1, y1, kDashboardBlack);
        surface.drawLine(x0, static_cast<int16_t>(y0 + 1), x1,
                         static_cast<int16_t>(y1 + 1), kDashboardBlack);
    };

    stroke(static_cast<int16_t>(x + 1), static_cast<int16_t>(y + 5),
           static_cast<int16_t>(x + 6), y);
    stroke(static_cast<int16_t>(x + 6), y, static_cast<int16_t>(x + 12), y);
    stroke(static_cast<int16_t>(x + 12), y, static_cast<int16_t>(x + 17),
           static_cast<int16_t>(y + 5));

    stroke(static_cast<int16_t>(x + 4), static_cast<int16_t>(y + 10),
           static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 7));
    stroke(static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 7),
           static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 7));
    stroke(static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 7),
           static_cast<int16_t>(x + 14), static_cast<int16_t>(y + 10));

    stroke(static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 14),
           static_cast<int16_t>(x + 9), static_cast<int16_t>(y + 12));
    stroke(static_cast<int16_t>(x + 9), static_cast<int16_t>(y + 12),
           static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 14));
    surface.fillRect(static_cast<int16_t>(x + 8), static_cast<int16_t>(y + 16), 2, 2,
                     kDashboardBlack);
}

void drawPrototypeBatteryIcon(IDrawSurface &surface, int16_t x, int16_t y) {
    surface.drawRect(x, y, 16, 9, kDashboardBlack);
    surface.fillRect(static_cast<int16_t>(x + 16), static_cast<int16_t>(y + 3), 2, 3,
                     kDashboardBlack);
    surface.fillRect(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 3), 10, 3,
                     kDashboardAccent);
}

void drawPrototypePageIcon(IDrawSurface &surface, PageIconKind icon, int16_t x, int16_t y) {
    switch (icon) {
        case PageIconKind::Overview:
            surface.drawRect(x, y, 14, 14, kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 5),
                             static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 5),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 9),
                             static_cast<int16_t>(x + 9), static_cast<int16_t>(y + 9),
                             kDashboardAccent);
            break;
        case PageIconKind::Month:
            surface.drawRect(x, static_cast<int16_t>(y + 1), 15, 13, kDashboardBlack);
            surface.drawLine(x, static_cast<int16_t>(y + 5), static_cast<int16_t>(x + 14),
                             static_cast<int16_t>(y + 5), kDashboardBlack);
            surface.fillRect(static_cast<int16_t>(x + 8), static_cast<int16_t>(y + 8), 4, 4,
                             kDashboardAccent);
            break;
        case PageIconKind::Week:
            surface.drawLine(static_cast<int16_t>(x + 2), static_cast<int16_t>(y + 13),
                             static_cast<int16_t>(x + 2), static_cast<int16_t>(y + 2),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 2), static_cast<int16_t>(y + 2),
                             static_cast<int16_t>(x + 13), static_cast<int16_t>(y + 2),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 4), static_cast<int16_t>(y + 11),
                             static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 8),
                             kDashboardAccent);
            surface.drawLine(static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 8),
                             static_cast<int16_t>(x + 10), static_cast<int16_t>(y + 9),
                             kDashboardAccent);
            surface.drawLine(static_cast<int16_t>(x + 10), static_cast<int16_t>(y + 9),
                             static_cast<int16_t>(x + 13), static_cast<int16_t>(y + 4),
                             kDashboardAccent);
            break;
        case PageIconKind::Clock:
            surface.drawRect(static_cast<int16_t>(x + 3), y, 9, 15, kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 7),
                             static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 3),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 7),
                             static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 9),
                             kDashboardAccent);
            break;
        case PageIconKind::News:
            surface.drawRect(x, static_cast<int16_t>(y + 2), 15, 12, kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 5),
                             static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 5),
                             kDashboardAccent);
            surface.drawLine(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 9),
                             static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 9),
                             kDashboardBlack);
            break;
        case PageIconKind::Weather:
            surface.drawRect(static_cast<int16_t>(x + 1), static_cast<int16_t>(y + 7), 8, 6,
                             kDashboardAccent);
            surface.drawLine(static_cast<int16_t>(x + 8), static_cast<int16_t>(y + 7),
                             static_cast<int16_t>(x + 15), static_cast<int16_t>(y + 7),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 5), static_cast<int16_t>(y + 13),
                             static_cast<int16_t>(x + 16), static_cast<int16_t>(y + 13),
                             kDashboardBlack);
            break;
        case PageIconKind::Portfolio:
            surface.drawRect(static_cast<int16_t>(x + 1), static_cast<int16_t>(y + 4), 14, 10,
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 5), static_cast<int16_t>(y + 4),
                             static_cast<int16_t>(x + 5), static_cast<int16_t>(y + 1),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 5), static_cast<int16_t>(y + 1),
                             static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 1),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 1),
                             static_cast<int16_t>(x + 11), static_cast<int16_t>(y + 4),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 4), static_cast<int16_t>(y + 10),
                             static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 7),
                             kDashboardAccent);
            surface.drawLine(static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 7),
                             static_cast<int16_t>(x + 10), static_cast<int16_t>(y + 9),
                             kDashboardAccent);
            surface.drawLine(static_cast<int16_t>(x + 10), static_cast<int16_t>(y + 9),
                             static_cast<int16_t>(x + 13), static_cast<int16_t>(y + 5),
                             kDashboardAccent);
            break;
        case PageIconKind::Economic:
            surface.drawRect(x, static_cast<int16_t>(y + 2), 15, 12, kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 4), static_cast<int16_t>(y + 11),
                             static_cast<int16_t>(x + 4), static_cast<int16_t>(y + 7),
                             kDashboardAccent);
            surface.drawLine(static_cast<int16_t>(x + 8), static_cast<int16_t>(y + 11),
                             static_cast<int16_t>(x + 8), static_cast<int16_t>(y + 5),
                             kDashboardBlack);
            surface.drawLine(static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 11),
                             static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 3),
                             kDashboardBlack);
            break;
    }
}

void drawStatusLine(IDrawSurface &surface, const std::string &statusText) {
    if (statusText.empty()) {
        return;
    }
    const size_t batPos = statusText.find("BAT:");
    std::string wifiSource = batPos == std::string::npos ? statusText : statusText.substr(0, batPos);
    while (!wifiSource.empty() && wifiSource.back() == ' ') {
        wifiSource.pop_back();
    }
    const std::string wifiText = fitText(surface, wifiSource, 220, 1);
    const std::string batText = batPos == std::string::npos ? std::string()
                                                            : fitText(surface, statusText.substr(batPos), 76, 1);

    drawWifiIcon(surface, kTitleX, static_cast<int16_t>(kStatusY - 1), kDashboardBlack);
    if (!wifiText.empty()) {
        surface.drawText(kStatusLeftTextX, kStatusY, wifiText,
                         kDashboardBlack, TextAlign::Left, 1);
    }
    if (!batText.empty()) {
        drawBatteryIcon(surface, kBatteryIconX, static_cast<int16_t>(kStatusY - 1), kDashboardBlack);
        surface.drawText(kBatteryTextX, kStatusY, batText, kDashboardBlack, TextAlign::Left, 1);
    }
}

void drawHeaderCore(IDrawSurface &surface, const std::string &title, const std::string &rightText,
                    const std::string &statusText, int16_t accentWidth) {
    surface.fillScreen(kDashboardWhite);
    drawHeaderIcon(surface);
    const bool hasRightText = !rightText.empty();
    const int16_t measuredRightWidth = hasRightText ? surface.measureText(rightText, 1) : 0;
    const int16_t rightWidth = hasRightText
        ? std::min<int16_t>(208, std::max<int16_t>(kRightWidth, static_cast<int16_t>(measuredRightWidth + 2)))
        : 0;
    const int16_t rightStart = hasRightText && measuredRightWidth > kRightWidth
        ? static_cast<int16_t>(kHeaderRightX - rightWidth)
        : (hasRightText ? kHeaderRightStartX : static_cast<int16_t>(surface.width() - 18));
    const int16_t titleMaxWidth = static_cast<int16_t>(rightStart - kTitleX - 12);
    const uint8_t titleSize = surface.measureText(title, 2) <= titleMaxWidth ? 2 : 1;
    surface.drawText(kTitleX, kTitleY, fitText(surface, title, titleMaxWidth, titleSize),
                     kDashboardBlack, TextAlign::Left, titleSize);
    if (hasRightText) {
        surface.drawText(kHeaderRightX, 16,
                         fitText(surface, rightText, rightWidth, 1),
                         kDashboardBlack, TextAlign::Right, 1);
    }
    drawStatusLine(surface, statusText);
    surface.drawLine(18, kHeaderRuleY, static_cast<int16_t>(surface.width() - 18), kHeaderRuleY,
                     kDashboardBlack);
    surface.drawLine(18, kHeaderAccentY, static_cast<int16_t>(18 + accentWidth), kHeaderAccentY,
                     kDashboardAccent);
}

void drawMetricCard(IDrawSurface &surface, const Rect &cell, const LabelValue &item) {
    box(surface, cell, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(cell.x + 6), static_cast<int16_t>(cell.y + 12),
                     item.label, kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(cell.x + cell.w / 2), static_cast<int16_t>(cell.y + 32),
                     item.value, item.accent ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Center, 2);
    if (!item.detail.empty()) {
        surface.drawText(static_cast<int16_t>(cell.x + 6), static_cast<int16_t>(cell.y + cell.h - 6),
                         item.detail, kDashboardBlack, TextAlign::Left, 1);
    }
}

void drawListMarker(IDrawSurface &surface, int16_t x, int16_t baselineY) {
    surface.fillRect(x, static_cast<int16_t>(baselineY - 7), 2, 10, kDashboardAccent);
    surface.fillRect(static_cast<int16_t>(x + 3), static_cast<int16_t>(baselineY - 7),
                     6, 1, kDashboardAccent);
}
}  // namespace

std::string fitText(IDrawSurface &surface, const std::string &text,
                    int16_t maxWidth, uint8_t size) {
    if (text.empty() || surface.measureText(text, size) <= maxWidth) {
        return text;
    }
    const std::string suffix = "..";
    if (surface.measureText(suffix, size) > maxWidth) {
        return "";
    }
    std::string best = suffix;
    for (size_t len = 1; len <= text.size(); ++len) {
        const std::string candidate = text.substr(0, len) + suffix;
        if (surface.measureText(candidate, size) > maxWidth) {
            break;
        }
        best = candidate;
    }
    return best;
}

void drawPrototypePageChrome(IDrawSurface &surface, const std::string &title,
                             PageIconKind icon, size_t pageNumber,
                             size_t pageCount, const std::string &timeText,
                             const std::string &ipText,
                             const std::string &batteryText) {
    surface.fillScreen(kDashboardWhite);
    drawPrototypePageIcon(surface, icon, 18, 13);

    const int16_t titleMaxWidth = 224;
    const uint8_t titleSize = surface.measureText(title, 2) <= titleMaxWidth ? 2 : 1;
    surface.drawText(42, 18, fitText(surface, title, titleMaxWidth, titleSize),
                     kDashboardBlack, TextAlign::Left, titleSize);

    drawPrototypeWifiIcon(surface, 282, 8);
    surface.drawLine(310, 12, 310, 25, kDashboardBlack);
    drawPrototypeBatteryIcon(surface, 326, 12);
    surface.drawText(382, 14, fitText(surface, batteryText, 46, 1),
                     kDashboardBlack, TextAlign::Right, 1);
    surface.drawText(382, 34, fitText(surface, timeText, 146, 1),
                     kDashboardBlack, TextAlign::Right, 1);

    surface.drawLine(18, kHeaderRuleY, static_cast<int16_t>(surface.width() - 18), kHeaderRuleY,
                     kDashboardBlack);
    surface.drawLine(18, kHeaderAccentY, 92, kHeaderAccentY, kDashboardAccent);

    surface.drawText(18, 286, fitText(surface, ipText, 170, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    const std::string pageText = std::to_string(pageNumber) + " | " + std::to_string(pageCount);
    surface.drawText(382, 286, pageText, kDashboardBlack, TextAlign::Right, 1);
}

void drawPageHeader(IDrawSurface &surface, const std::string &title,
                    const std::string &rightText, const std::string &statusText,
                    int16_t accentWidth) {
    drawHeaderCore(surface, title, rightText, statusText, accentWidth);
}

void drawHeader(IDrawSurface &surface, const std::string &title,
                const std::string &subtitle, const std::string &rightText) {
    drawHeaderCore(surface, title, rightText, subtitle, 74);
}

void drawPanel(IDrawSurface &surface, const Rect &rect, const std::string &title,
               const std::vector<LabelValue> &items) {
    fill(surface, rect, kDashboardWhite);
    box(surface, rect, kDashboardBlack);
    if (!title.empty()) {
        surface.drawText(static_cast<int16_t>(rect.x + 6), static_cast<int16_t>(rect.y + 14),
                         title, kDashboardAccent, TextAlign::Left, 1);
        surface.drawLine(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 20),
                         static_cast<int16_t>(rect.x + rect.w - 5), static_cast<int16_t>(rect.y + 20),
                         kDashboardBlack);
    }
    int16_t y = static_cast<int16_t>(rect.y + 30);
    for (const auto &item : items) {
        surface.drawText(static_cast<int16_t>(rect.x + 6), y, item.label, kDashboardBlack,
                         TextAlign::Left, 1);
        surface.drawText(static_cast<int16_t>(rect.x + rect.w - 6), y, item.value,
                         item.accent ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Right, 1);
        if (!item.detail.empty()) {
            surface.drawText(static_cast<int16_t>(rect.x + 6), static_cast<int16_t>(y + 10),
                             item.detail, kDashboardBlack, TextAlign::Left, 1);
        }
        y = static_cast<int16_t>(y + 24);
    }
}

void drawList(IDrawSurface &surface, const Rect &rect, const std::string &title,
              const std::vector<std::string> &items, bool accentTitle) {
    fill(surface, rect, kDashboardWhite);
    box(surface, rect, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 6), static_cast<int16_t>(rect.y + 10),
                     title, accentTitle ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 1);
    surface.drawLine(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 20),
                     static_cast<int16_t>(rect.x + rect.w - 5), static_cast<int16_t>(rect.y + 20),
                     kDashboardBlack);
    const int16_t availableH = static_cast<int16_t>(rect.h - 28);
    const int16_t rowH = items.empty()
        ? 18
        : std::max<int16_t>(18, static_cast<int16_t>(availableH / static_cast<int16_t>(items.size())));
    int16_t y = static_cast<int16_t>(rect.y + 35);
    for (const auto &item : items) {
        surface.fillRect(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(y - 12),
                         static_cast<int16_t>(rect.w - 8), std::min<int16_t>(rowH, 18),
                         kDashboardWhite);
        drawListMarker(surface, static_cast<int16_t>(rect.x + 8), y);
        surface.drawText(static_cast<int16_t>(rect.x + 8 + 15), y - 4,
                         fitText(surface, item, static_cast<int16_t>(rect.w - 34), 1),
                         kDashboardBlack,
                         TextAlign::Left, 1);
        y = static_cast<int16_t>(y + rowH);
    }
}

void drawMetricRow(IDrawSurface &surface, const Rect &rect, const std::vector<LabelValue> &items) {
    if (items.empty()) {
        return;
    }
    const int16_t cellW = rect.w / static_cast<int16_t>(items.size());
    for (size_t i = 0; i < items.size(); ++i) {
        const Rect cell{static_cast<int16_t>(rect.x + cellW * static_cast<int16_t>(i)), rect.y,
                        cellW, rect.h};
        drawMetricCard(surface, cell, items[i]);
    }
}

void drawMetricGrid(IDrawSurface &surface, const Rect &rect, const std::vector<LabelValue> &items,
                    uint8_t columns) {
    if (items.empty()) {
        return;
    }
    const uint8_t safeColumns = std::max<uint8_t>(1, columns);
    const int rows = static_cast<int>((items.size() + safeColumns - 1) / safeColumns);
    const int16_t cellW = rect.w / static_cast<int16_t>(safeColumns);
    const int16_t cellH = rect.h / static_cast<int16_t>(rows);
    for (size_t i = 0; i < items.size(); ++i) {
        const int row = static_cast<int>(i / safeColumns);
        const int col = static_cast<int>(i % safeColumns);
        const Rect cell{static_cast<int16_t>(rect.x + cellW * col),
                        static_cast<int16_t>(rect.y + cellH * row),
                        cellW, cellH};
        drawMetricCard(surface, cell, items[i]);
    }
}

void drawSectionMarker(IDrawSurface &surface, int16_t x, int16_t y, uint16_t color) {
    surface.fillRect(x, y, 3, 12, color);
    surface.fillRect(static_cast<int16_t>(x + 5), y, 8, 2, color);
}

void drawMonthGrid(IDrawSurface &surface, const Rect &rect,
                   const std::vector<std::string> &cells,
                   const std::string &monthLabel) {
    fill(surface, rect, kDashboardWhite);
    box(surface, rect, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 6), static_cast<int16_t>(rect.y + 14),
                     monthLabel, kDashboardAccent, TextAlign::Left, 1);
    const int16_t gridX = static_cast<int16_t>(rect.x + 4);
    const int16_t gridY = static_cast<int16_t>(rect.y + 22);
    const int16_t cellW = rect.w / 7;
    const int16_t cellH = static_cast<int16_t>((rect.h - 24) / 6);
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            const int index = row * 7 + col;
            const Rect cell{static_cast<int16_t>(gridX + col * cellW),
                            static_cast<int16_t>(gridY + row * cellH),
                            cellW, cellH};
            surface.drawRect(cell.x, cell.y, cell.w, cell.h, kDashboardBlack);
            if (index < static_cast<int>(cells.size()) && !cells[index].empty()) {
                surface.drawText(static_cast<int16_t>(cell.x + 3), static_cast<int16_t>(cell.y + 12),
                                 cells[index], kDashboardBlack, TextAlign::Left, 1);
            }
        }
    }
}

void drawTimeline(IDrawSurface &surface, const Rect &rect,
                  const std::vector<LabelValue> &items) {
    fill(surface, rect, kDashboardWhite);
    box(surface, rect, kDashboardBlack);
    if (items.empty()) {
        return;
    }
    const int16_t rowH = std::max<int16_t>(18, static_cast<int16_t>((rect.h - 4) / items.size()));
    for (size_t i = 0; i < items.size(); ++i) {
        const int16_t y = static_cast<int16_t>(rect.y + 4 + rowH * static_cast<int16_t>(i));
        surface.drawLine(static_cast<int16_t>(rect.x + 4), y + rowH - 2,
                         static_cast<int16_t>(rect.x + rect.w - 4), y + rowH - 2, kDashboardBlack);
        surface.drawText(static_cast<int16_t>(rect.x + 6), y, items[i].label, kDashboardAccent,
                         TextAlign::Left, 1);
        surface.drawText(static_cast<int16_t>(rect.x + 56), y, items[i].value, kDashboardBlack,
                         TextAlign::Left, 1);
        if (!items[i].detail.empty()) {
            surface.drawText(static_cast<int16_t>(rect.x + rect.w - 6), y, items[i].detail,
                             items[i].accent ? kDashboardAccent : kDashboardBlack,
                             TextAlign::Right, 1);
        }
    }
}

}  // namespace calm_grid
