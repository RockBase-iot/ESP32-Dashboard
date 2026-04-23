#pragma once

#include "ui/pages/page_error.h"

// PageError800x480 — error / low-battery page for the 800×480 EPD.
class PageError800x480 final : public PageErrorBase {
public:
    void        create(Adafruit_GFX &gfx,
                       uint16_t w, uint16_t h,
                       uint16_t colorAccent,
                       bool hasAccent) override;
    void        draw()          override;
    const char *name()    const override { return "Error800x480"; }
};
