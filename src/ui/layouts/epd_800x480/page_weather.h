#pragma once

#include "ui/pages/page_weather.h"

// PageWeather800x480 — weather page for the 800×480 EPD (7.5" BW/3C/7C).
// Skeleton only — TODO: implement drawing methods when artwork is ready.
class PageWeather800x480 final : public PageWeatherBase {
public:
    void        create(Adafruit_GFX &gfx,
                       uint16_t w, uint16_t h,
                       uint16_t colorAccent,
                       bool hasAccent) override;
    void        draw()          override;
    const char *name()    const override { return "Weather800x480"; }

protected:
    void _drawCurrentConditions() override;
    void _drawForecast()          override;
    void _drawStatusBar()         override;
    void _drawSunriseSunset()     override;
    void _drawAqi()               override;
};
