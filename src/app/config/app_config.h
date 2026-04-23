#pragma once

#include <Arduino.h>

// AppConfig — flat struct holding all runtime configuration.
//
// Populated once at startup by loadAppConfig() from NVS (with defaults as
// fallback). The web config portal calls saveAppConfig() to persist changes.
// The rest of the application reads this struct; it never touches NVS directly.
struct AppConfig {
    // WiFi
    String wifiSsid;
    String wifiPassword;

    // Location (used to build Open-Meteo request URLs — no API key required)
    String lat;
    String lon;
    String city;

    // Time
    int      utcOffset;    // UTC offset in hours, e.g. 8 = UTC+8, -5 = UTC-5
    String   timeFormat;   // strftime format, e.g. "%H:%M"
    String   dateFormat;   // strftime format, e.g. "%a, %B %e"
    int      sleepDuration; // minutes between wake cycles
    int      bedTime;       // hour (24-h) to pause updates
    int      wakeTime;      // hour (24-h) to resume updates

    // Units
    String unitsTemp;    // "C" / "F"
    String unitsSpeed;   // "ms" / "mph" / "kmh" / "kn"
    String unitsPres;    // "hPa" / "inHg" / "mmHg"
    String unitsDist;    // "km" / "mi"
    String unitsPrecip;  // "mm" / "in"

    // Locale
    String language;     // "en_US" / "zh_CN" / ...
};

// Load config from NVS into cfg. Missing keys use the defaults in nvs_table.h.
void loadAppConfig(AppConfig &cfg);

// Persist cfg back to NVS (called by the web config portal on save).
void saveAppConfig(const AppConfig &cfg);
