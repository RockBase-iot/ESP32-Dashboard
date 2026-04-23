#pragma once

#include "ui/pages/page_error.h"

// PageError400x300 — error / low-battery page for the 400×300 EPD.
class PageError400x300 final : public PageErrorBase {
public:
    void        create(Adafruit_GFX &gfx,
                       uint16_t w, uint16_t h,
                       uint16_t colorAccent,
                       bool hasAccent) override;
    void        draw()          override;
    const char *name()    const override { return "Error400x300"; }
};
