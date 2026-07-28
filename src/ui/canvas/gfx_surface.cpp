#include "gfx_surface.h"

#include <Adafruit_GFX.h>

GfxSurface::GfxSurface(Adafruit_GFX &gfx) : _gfx(gfx) {}

int16_t GfxSurface::width() const {
    return _gfx.width();
}

int16_t GfxSurface::height() const {
    return _gfx.height();
}

void GfxSurface::drawPixel(int16_t x, int16_t y, uint16_t color) {
    _gfx.drawPixel(x, y, color);
}

void GfxSurface::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    _gfx.drawLine(x0, y0, x1, y1, color);
}

void GfxSurface::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _gfx.drawRect(x, y, w, h, color);
}

void GfxSurface::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _gfx.fillRect(x, y, w, h, color);
}

void GfxSurface::fillScreen(uint16_t color) {
    _gfx.fillScreen(color);
}

void GfxSurface::drawText(int16_t x, int16_t y, const std::string &text, uint16_t color,
                          TextAlign align, uint8_t size) {
    _gfx.setFont();
    _gfx.setTextSize(size);
    _gfx.setTextWrap(false);
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    _gfx.getTextBounds(text.c_str(), x, y, &x1, &y1, &w, &h);
    int16_t drawX = x;
    if (align == TextAlign::Center) {
        drawX = x - static_cast<int16_t>(w / 2);
    } else if (align == TextAlign::Right) {
        drawX = x - static_cast<int16_t>(w);
    }
    _gfx.setTextColor(color);
    _gfx.setCursor(drawX, y);
    _gfx.print(text.c_str());
}

int16_t GfxSurface::measureText(const std::string &text, uint8_t size) const {
    const_cast<Adafruit_GFX &>(_gfx).setFont();
    const_cast<Adafruit_GFX &>(_gfx).setTextSize(size);
    const_cast<Adafruit_GFX &>(_gfx).setTextWrap(false);
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    _gfx.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
    return static_cast<int16_t>(w);
}
