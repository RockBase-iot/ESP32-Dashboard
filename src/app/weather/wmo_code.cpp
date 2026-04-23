#include "wmo_code.h"

// WMO weather interpretation codes
// https://open-meteo.com/en/docs#weathervariables

const char *wmo_weather_text(int code) {
    switch (code) {
        case 0:  return "Clear sky";
        case 1:  return "Mainly clear";
        case 2:  return "Partly cloudy";
        case 3:  return "Overcast";
        case 45: return "Foggy";
        case 48: return "Depositing rime fog";
        case 51: return "Light drizzle";
        case 53: return "Moderate drizzle";
        case 55: return "Dense drizzle";
        case 61: return "Slight rain";
        case 63: return "Moderate rain";
        case 65: return "Heavy rain";
        case 71: return "Slight snow";
        case 73: return "Moderate snow";
        case 75: return "Heavy snow";
        case 77: return "Snow grains";
        case 80: return "Slight showers";
        case 81: return "Moderate showers";
        case 82: return "Violent showers";
        case 85: return "Slight snow showers";
        case 86: return "Heavy snow showers";
        case 95: return "Thunderstorm";
        case 96: return "Thunderstorm w/ hail";
        case 99: return "Thunderstorm w/ heavy hail";
        default: return "Unknown";
    }
}

const char *wmo_weather_icon(int code, bool is_day) {
    // TODO: map WMO codes to FontAwesome 5 Solid UTF-8 glyphs.
    (void)is_day;
    (void)code;
    return "\uf185"; // fa-sun placeholder
}

uint32_t wmo_weather_icon_color(int code, bool is_day) {
    // TODO: return appropriate RGB888 color per weather condition.
    (void)code;
    (void)is_day;
    return 0xFFAA00; // amber placeholder
}

const uint8_t *wmo_get_bitmap_96(int code, bool is_day) {
    // TODO: return pointer into src/assets/icons/96x96/ bitmaps.
    (void)code;
    (void)is_day;
    return nullptr;
}

const uint8_t *wmo_get_bitmap_196(int code, bool is_day) {
    // TODO: return pointer into src/assets/icons/196x196/ bitmaps.
    (void)code;
    (void)is_day;
    return nullptr;
}
