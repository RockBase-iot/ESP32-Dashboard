#pragma once

// LocaleData — all locale-specific strings needed by the UI layer.
//
// One const instance per language lives in its own .cpp file.
// The UI layer receives a const LocaleData& from getLocale() and reads
// the fields it needs; it never links against a specific language directly.
struct LocaleData {
    // LC_TIME fields (used by strftime / _strftime wrappers).
    const char *d_t_fmt;     // full date + time format
    const char *d_fmt;       // date-only format
    const char *t_fmt;       // time-only format
    const char *t_fmt_ampm;  // 12-hour time format
    const char *am_str;
    const char *pm_str;
    const char *day[7];      // full weekday names, Sunday first
    const char *abday[7];    // abbreviated weekday names
    const char *mon[12];     // full month names
    const char *abmon[12];   // abbreviated month names

    // Open-Meteo API language code (passed as &language= query param).
    const char *owm_lang;    // "en" / "zh" / "de" / ...

    // UI text strings.
    const char *txt_feels_like;
    const char *txt_sunrise;
    const char *txt_sunset;
    const char *txt_wind;
    const char *txt_humidity;
    const char *txt_uv_index;
    const char *txt_pressure;
    const char *txt_air_quality;
    const char *txt_visibility;
    const char *txt_indoor_temperature;
    const char *txt_no_data;
    const char *txt_loading;
    const char *txt_wifi_error;
    const char *txt_weather_error;
};
