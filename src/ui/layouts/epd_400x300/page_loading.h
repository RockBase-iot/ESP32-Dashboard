#pragma once

#include "ui/pages/page_loading.h"

// PageLoading400x300 — startup / refreshing page for the 400×300 EPD.
class PageLoading400x300 final : public PageLoadingBase {
public:
    void        create(Adafruit_GFX &gfx,
                       uint16_t w, uint16_t h,
                       uint16_t colorAccent,
                       bool hasAccent) override;
    void        draw()          override;
    const char *name()    const override { return "Loading400x300"; }
};
