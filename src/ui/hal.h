#pragma once

#include <Adafruit_GFX.h>
#include <stdint.h>

// UIPage — base class for all EPD pages (no LVGL, draws via Adafruit_GFX).
//
// Usage pattern inside a DashboardApp rendering pass:
//   page.create(board.gfx(), board.dispWidth(), board.dispHeight(),
//               board.colorAccent(), board.hasAccentColor());
//   board.epd().firstPage();
//   do { page.draw(); } while (board.epd().nextPage());
//   board.epd().hibernate();
class UIPage {
public:
    virtual ~UIPage() = default;

    // Initialize the page with GFX context and display capabilities.
    // Must be called before the first draw() invocation.
    virtual void create(Adafruit_GFX &gfx,
                        uint16_t w, uint16_t h,
                        uint16_t colorAccent,
                        bool hasAccent) = 0;

    // Render the full page content. Called once per GxEPD2 page inside the
    // firstPage()/nextPage() loop.
    virtual void draw() = 0;

    // Short human-readable page name used for logging.
    virtual const char *name() const = 0;
};
