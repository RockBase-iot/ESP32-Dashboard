#include "page_loading.h"

void PageLoading800x480::create(Adafruit_GFX &gfx,
                                 uint16_t w, uint16_t h,
                                 uint16_t colorAccent,
                                 bool hasAccent) {
    _gfx         = &gfx;
    _w           = w;
    _h           = h;
    _colorAccent = colorAccent;
    _hasAccent   = hasAccent;
}

void PageLoading800x480::draw() { /* TODO */ }
