#pragma once

#include "ui/pages/page_weather.h"

// PageWeather400x300 — weather page for the 400×300 3-color EPD.
//
// Only coordinates, font sizes, and icon dimensions live here.
// All business logic is in PageWeatherBase.
class PageWeather400x300 final : public PageWeatherBase {
public:
    void        create(Adafruit_GFX &gfx,
                       uint16_t w, uint16_t h,
                       uint16_t colorAccent,
                       bool hasAccent) override;
    void        draw()          override;
    const char *name()    const override { return "Weather400x300"; }

protected:
    void _drawCurrentConditions() override;
    void _drawForecast()          override;
    void _drawStatusBar()         override;
    // Consolidated data rows: replaces _drawSunriseSunset() + _drawAqi()
    void _drawDataRows();
    void _drawHourlyGraph();

    // These are no longer used but must be declared to satisfy the base class:
    void _drawSunriseSunset()     override {}
    void _drawAqi()               override {}
};
