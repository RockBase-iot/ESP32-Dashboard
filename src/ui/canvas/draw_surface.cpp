#include "draw_surface.h"

#include <cmath>

MemoryDrawSurface::MemoryDrawSurface(int16_t width, int16_t height)
    : _width(width), _height(height) {}

bool MemoryDrawSurface::isAllowedColor(uint16_t color) const {
    return color == kDashboardBlack || color == kDashboardWhite || color == kDashboardAccent;
}

void MemoryDrawSurface::touchPixel(int16_t x, int16_t y, uint16_t color) {
    if (!isAllowedColor(color)) {
        ++_invalidColor;
    }
    if (x < 0 || y < 0 || x >= _width || y >= _height) {
        ++_outOfBounds;
    }
}

void MemoryDrawSurface::drawPixel(int16_t x, int16_t y, uint16_t color) {
    touchPixel(x, y, color);
}

void MemoryDrawSurface::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t dx = std::abs(static_cast<int>(x1 - x0));
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -std::abs(static_cast<int>(y1 - y0));
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int16_t e2 = static_cast<int16_t>(2 * err);
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void MemoryDrawSurface::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    fillRect(x, y, w, 1, color);
    fillRect(x, y + h - 1, w, 1, color);
    fillRect(x, y, 1, h, color);
    fillRect(x + w - 1, y, 1, h, color);
}

void MemoryDrawSurface::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) {
        return;
    }
    for (int16_t yy = 0; yy < h; ++yy) {
        for (int16_t xx = 0; xx < w; ++xx) {
            touchPixel(static_cast<int16_t>(x + xx), static_cast<int16_t>(y + yy), color);
        }
    }
}

void MemoryDrawSurface::fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

void MemoryDrawSurface::drawText(int16_t x, int16_t y, const std::string &text, uint16_t color,
                                 TextAlign align, uint8_t size) {
    _textOps.push_back(TextOp{x, y, text, color, align, size});
    const int16_t textW = measureText(text, size);
    const int16_t textH = static_cast<int16_t>(8 * size);
    int16_t drawX = x;
    if (align == TextAlign::Center) {
        drawX = static_cast<int16_t>(x - textW / 2);
    } else if (align == TextAlign::Right) {
        drawX = static_cast<int16_t>(x - textW);
    }
    fillRect(drawX, static_cast<int16_t>(y - textH + 1), textW, textH, color);
}

int16_t MemoryDrawSurface::measureText(const std::string &text, uint8_t size) const {
    return static_cast<int16_t>(text.size() * 6 * size);
}
