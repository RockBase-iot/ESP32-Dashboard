#include "page_error.h"
#include "assets/fonts/FreeSans.h"
#include "assets/icons/icons_96x96.h"

void PageError400x300::create(Adafruit_GFX &gfx,
                               uint16_t w, uint16_t h,
                               uint16_t colorAccent,
                               bool hasAccent) {
    _gfx         = &gfx;
    _w           = w;
    _h           = h;
    _colorAccent = colorAccent;
    _hasAccent   = hasAccent;
}

void PageError400x300::draw() {
    const uint16_t iconSz = 96;
    const int16_t  cx     = _w / 2;
    const int16_t  cy     = _h / 2;

    // 96×96 error icon，居中展示（参考 DISP_3C_E420 分支）
    uint16_t iconColor = _hasAccent ? _colorAccent : 0x0000;
    _gfx->drawBitmap(cx - iconSz / 2,
                     cy - iconSz / 2 - 10,
                     error_icon_96x96, iconSz, iconSz,
                     0xFFFF, iconColor); // bg=white, fg=accent

    // 标题文字
    _gfx->setFont(&FONT_16pt8b);
    _gfx->setTextColor(0x0000);
    if (_title) {
        int16_t tx, ty;
        uint16_t tw, th;
        _gfx->getTextBounds(_title, 0, 0, &tx, &ty, &tw, &th);
        _gfx->setCursor(cx - tw / 2, cy + iconSz / 2 + 11 + th);
        _gfx->print(_title);
    }

    // 正文小字
    if (_body) {
        _gfx->setFont(&FONT_7pt8b);
        int16_t bx, by;
        uint16_t bw, bh;
        _gfx->getTextBounds(_body, 0, 0, &bx, &by, &bw, &bh);
        _gfx->setCursor(cx - bw / 2, cy + iconSz / 2 + 11 + 28 + bh);
        _gfx->print(_body);
    }
}
