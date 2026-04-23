#pragma once

#include <cmath>
#include "ui/hal.h"
#include "app/weather/weather_data.h"
#include "app/locale/locale_data.h"
#include "app/config/app_config.h"

// PageWeatherBase — abstract base for the weather page.
//
// Responsibilities:
//   - Hold references to weather/AQI/locale/config data.
//   - Declare pure-virtual drawing primitives that subclasses implement with
//     resolution-specific coordinates and fonts.
//
// Subclasses (in ui/layouts/epd_NxM/) implement create(), draw(), and every
// _drawXxx() method using the coordinate constants from their widgets.h.
class PageWeatherBase : public UIPage {
public:
    // Inject current data before calling draw().
    void setWeatherData(const WeatherData    &weather,
                        const AirQualityData &aqi,
                        const LocaleData     &loc,
                        const AppConfig      &cfg);

    // Optional: inject indoor sensor reading (NaN = unavailable).
    void setIndoorData(float tempC, float humi) {
        _indoorTemp = tempC;
        _indoorHumi = humi;
    }

    // Optional: inject local IP (call before WiFi is disconnected).
    void setLocalIP(const String &ip) { _localIP = ip; }

protected:
    // Drawing primitives — layout/font/coordinate details are in subclasses.
    virtual void _drawCurrentConditions() = 0;
    virtual void _drawForecast()          = 0;
    virtual void _drawStatusBar()         = 0;
    virtual void _drawSunriseSunset()     = 0;
    virtual void _drawAqi()               = 0;

    // Data pointers set by setWeatherData().
    const WeatherData    *_weather = nullptr;
    const AirQualityData *_aqi     = nullptr;
    const LocaleData     *_loc     = nullptr;
    const AppConfig      *_cfg     = nullptr;

    // Indoor sensor data (NaN = unavailable).
    float _indoorTemp = NAN;
    float _indoorHumi = NAN;

    // Local IP at render time.
    String _localIP;

    // GFX context set by create().
    Adafruit_GFX *_gfx = nullptr;
    uint16_t      _w           = 0;
    uint16_t      _h           = 0;
    uint16_t      _colorAccent = 0;
    bool          _hasAccent   = false;
};
