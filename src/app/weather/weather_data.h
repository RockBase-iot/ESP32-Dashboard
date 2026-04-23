#pragma once

#include <Arduino.h>
#include <vector>

// ─── Open-Meteo API endpoints ─────────────────────────────────────────────
#define WEATHER_API_HOST     "api.open-meteo.com"
#define AQI_API_HOST         "air-quality-api.open-meteo.com"
#define WEATHER_HTTP_TIMEOUT 4500   // ms

// ─── Weather data structures ──────────────────────────────────────────────

struct WeatherCurrent {
    float temperature;           // °C (convert in UI per unitsTemp)
    float apparent_temperature;  // °C
    float humidity;              // %
    float wind_speed;            // km/h (convert in UI per unitsSpeed)
    float wind_direction;        // degrees
    float pressure;              // hPa (convert in UI per unitsPres)
    float visibility;            // metres (open-meteo returns metres)
    int   weather_code;          // WMO weather interpretation code
    bool  is_day;
};

struct WeatherHourly {
    String time;                       // ISO 8601, e.g. "2024-01-01T14:00"
    float  temperature;                // °C
    int    weather_code;
    int    precipitation_probability;  // %
    int    humidity;                   // % relative humidity
};

struct WeatherDaily {
    String date;        // YYYY-MM-DD
    int    weather_code;
    float  temp_max;    // °C
    float  temp_min;    // °C
    float  uv_index_max;
    String sunrise;     // ISO 8601
    String sunset;      // ISO 8601
};

struct WeatherData {
    WeatherCurrent             current;
    std::vector<WeatherHourly> hourly;  // next 12 h
    std::vector<WeatherDaily>  daily;   // next 4 days (including today)
    String                     timezone;
    float                      elevation      = NAN; // metres above sea level (from 90m DEM)
    uint32_t                   last_update_ms = 0;
    bool                       valid          = false;
};

// ─── Air quality data structures ──────────────────────────────────────────

struct AirQualityData {
    float    pm2_5;           // μg/m³
    float    pm10;            // μg/m³
    int      us_aqi;
    int      european_aqi;
    int      daily_aqi[4];   // AQI at noon for the next 4 days
    float    daily_pm25[4];
    float    daily_pm10[4];
    uint32_t last_update_ms = 0;
    bool     valid          = false;
};
