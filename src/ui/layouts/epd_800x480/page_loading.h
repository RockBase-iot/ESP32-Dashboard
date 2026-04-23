#pragma once

#include "ui/pages/page_loading.h"

// PageLoading800x480 — startup / refreshing page for the 800×480 EPD.
class PageLoading800x480 final : public PageLoadingBase {
public:
    void        create(Adafruit_GFX &gfx,
                       uint16_t w, uint16_t h,
                       uint16_t colorAccent,
                       bool hasAccent) override;
    void        draw()          override;
    const char *name()    const override { return "Loading800x480"; }
};
