#pragma once

#include "ui/hal.h"

// PageLoadingBase — abstract base for the startup / refreshing page.
//
// Subclasses implement create() and draw() with resolution-specific layout.
class PageLoadingBase : public UIPage {
public:
    // Set status text before calling draw().
    void setStatus(const char *status);

protected:
    const char *_status = nullptr;

    Adafruit_GFX *_gfx = nullptr;
    uint16_t      _w           = 0;
    uint16_t      _h           = 0;
    uint16_t      _colorAccent = 0;
    bool          _hasAccent   = false;
};
