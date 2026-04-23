#include "page_loading.h"
#include "assets/fonts/FreeSans.h"
#include "assets/icons/icons_32x32.h"
#include <string.h>

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

    // 状态文字居中，支持 '\n' 换行（最多两行）
    if (_status) {
        _gfx->setFont(&FONT_9pt8b);
        _gfx->setTextColor(0x0000);
        int16_t tx, ty;
        uint16_t tw, th;

        const char *nl = strchr(_status, '\n');
        if (nl) {
            // 两行：分别居中，行间距 6px
            char line1[64];
            size_t len = static_cast<size_t>(nl - _status);
            if (len >= sizeof(line1)) len = sizeof(line1) - 1;
            memcpy(line1, _status, len);
            line1[len] = '\0';
            const char *line2 = nl + 1;

            _gfx->getTextBounds(line1, 0, 0, &tx, &ty, &tw, &th);
            int16_t lineH = th + 6;
            _gfx->setCursor(cx - (int16_t)(tw / 2), cy + 4 + th);
            _gfx->print(line1);

            _gfx->getTextBounds(line2, 0, 0, &tx, &ty, &tw, &th);
            _gfx->setCursor(cx - (int16_t)(tw / 2), cy + 4 + lineH + th);
            _gfx->print(line2);
        } else {
            _gfx->getTextBounds(_status, 0, 0, &tx, &ty, &tw, &th);
            _gfx->setCursor(cx - tw / 2, cy + 8 + th);
            _gfx->print(_status);
        }
    }
}
