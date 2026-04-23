#pragma once

#include "ui/hal.h"

// PageErrorBase — abstract base for the error / low-battery page.
//
// Subclasses implement create() and draw() with resolution-specific layout.
class PageErrorBase : public UIPage {
public:
    // Set the message to display before calling draw().
    void setMessage(const char *title, const char *body);

protected:
    const char *_title = nullptr;
    const char *_body  = nullptr;

    Adafruit_GFX *_gfx = nullptr;
    uint16_t      _w           = 0;
    uint16_t      _h           = 0;
    uint16_t      _colorAccent = 0;
    bool          _hasAccent   = false;
};
