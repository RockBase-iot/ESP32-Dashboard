#include "page_loading.h"
#include "assets/fonts/FreeSans.h"
#include "assets/icons/icons_32x32.h"

void PageLoading400x300::create(Adafruit_GFX &gfx,
                                 uint16_t w, uint16_t h,
                                 uint16_t colorAccent,
                                 bool hasAccent) {
    _gfx         = &gfx;
    _w           = w;
    _h           = h;
    _colorAccent = colorAccent;
    _hasAccent   = hasAccent;
}

void PageLoading400x300::draw() {
    const int16_t cx = _w / 2;
    const int16_t cy = _h / 2;

    // 刷新图标居中
    _gfx->drawBitmap(cx - 16, cy - 36,
                     wi_refresh_32x32, 32, 32,
                     0xFFFF, 0x0000);

    // 状态文字居中
    if (_status) {
        _gfx->setFont(&FONT_9pt8b);
        _gfx->setTextColor(0x0000);
        int16_t tx, ty;
        uint16_t tw, th;
        _gfx->getTextBounds(_status, 0, 0, &tx, &ty, &tw, &th);
        _gfx->setCursor(cx - tw / 2, cy + 8 + th);
        _gfx->print(_status);
    }
}
