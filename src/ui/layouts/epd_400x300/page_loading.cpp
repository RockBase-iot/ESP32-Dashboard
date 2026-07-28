#include "page_loading.h"
#include "assets/fonts/FreeSans.h"
#include "assets/icons/icons_32x32.h"
#include <string.h>

namespace {
constexpr uint8_t kMaxStatusLines = 4;
constexpr int16_t kStatusLineGap = 7;

uint8_t splitStatusLines(const char *text, char lines[][64]) {
    if (!text) {
        return 0;
    }
    uint8_t count = 0;
    const char *start = text;
    while (*start && count < kMaxStatusLines) {
        const char *end = strchr(start, '\n');
        const size_t rawLen = end ? static_cast<size_t>(end - start) : strlen(start);
        size_t len = rawLen;
        if (len >= 64) {
            len = 63;
        }
        memcpy(lines[count], start, len);
        lines[count][len] = '\0';
        ++count;
        if (!end) {
            break;
        }
        start = end + 1;
    }
    return count;
}
}  // namespace

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

    _gfx->drawBitmap(cx - 16, cy - 48,
                     wi_refresh_32x32, 32, 32,
                     0xFFFF, 0x0000);

    if (_status) {
        _gfx->setFont(&FONT_9pt8b);
        _gfx->setTextColor(0x0000);

        char lines[kMaxStatusLines][64];
        const uint8_t lineCount = splitStatusLines(_status, lines);
        uint16_t maxHeight = 0;
        int16_t tx, ty;
        uint16_t tw, th;
        for (uint8_t i = 0; i < lineCount; ++i) {
            _gfx->getTextBounds(lines[i], 0, 0, &tx, &ty, &tw, &th);
            if (th > maxHeight) {
                maxHeight = th;
            }
        }
        if (maxHeight == 0) {
            maxHeight = 12;
        }

        const int16_t totalHeight =
            static_cast<int16_t>(lineCount * maxHeight +
                                 (lineCount > 0 ? (lineCount - 1) * kStatusLineGap : 0));
        int16_t baseline = cy + 10 - totalHeight / 2 + maxHeight;
        for (uint8_t i = 0; i < lineCount; ++i) {
            _gfx->getTextBounds(lines[i], 0, 0, &tx, &ty, &tw, &th);
            _gfx->setCursor(cx - static_cast<int16_t>(tw / 2), baseline);
            _gfx->print(lines[i]);
            baseline += maxHeight + kStatusLineGap;
        }
    }
}
