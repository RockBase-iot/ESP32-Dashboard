#pragma once

#include <stdint.h>

// WMO weather interpretation code → text, icon, and color mappings.
// Reference: https://open-meteo.com/en/docs#weathervariables

// Human-readable description for the given WMO code.
// e.g. 0 → "Clear sky", 61 → "Slight rain"
const char *wmo_weather_text(int code);

// FontAwesome 5 Solid UTF-8 glyph for the given WMO code.
// is_day selects the day vs night variant where applicable.
const char *wmo_weather_icon(int code, bool is_day);

// Suggested RGB888 tint color for the icon (for color displays).
uint32_t wmo_weather_icon_color(int code, bool is_day);

// Bitmap pointer for a 96×96 icon matching the WMO code.
// Returns nullptr if no bitmap is available.
const uint8_t *wmo_get_bitmap_96(int code, bool is_day);

// Bitmap pointer for a 196×196 icon matching the WMO code.
// Returns nullptr if no bitmap is available.
const uint8_t *wmo_get_bitmap_196(int code, bool is_day);
