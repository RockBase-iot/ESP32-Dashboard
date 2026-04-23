#include "page_weather.h"
#include "widgets.h"

void PageWeather800x480::create(Adafruit_GFX &gfx,
                                uint16_t w, uint16_t h,
                                uint16_t colorAccent,
                                bool hasAccent) {
    _gfx         = &gfx;
    _w           = w;
    _h           = h;
    _colorAccent = colorAccent;
    _hasAccent   = hasAccent;
}

void PageWeather800x480::draw() {
    _drawStatusBar();
    _drawCurrentConditions();
    _drawSunriseSunset();
    _drawAqi();
    _drawForecast();
}

void PageWeather800x480::_drawStatusBar()         { /* TODO */ }
void PageWeather800x480::_drawCurrentConditions() { /* TODO */ }
void PageWeather800x480::_drawForecast()          { /* TODO */ }
void PageWeather800x480::_drawSunriseSunset()     { /* TODO */ }
void PageWeather800x480::_drawAqi()               { /* TODO */ }
