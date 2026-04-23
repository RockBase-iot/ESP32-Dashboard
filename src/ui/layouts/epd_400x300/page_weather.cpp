// Weather page layout for 400×300 3-color EPD.
// Layout matches reference project DISP_3C_E420 sections in renderer.cpp exactly.
// All labels use hardcoded English strings to ensure correct rendering with
// Latin bitmap fonts (locale strings may contain non-Latin characters).

#include "page_weather.h"
#include "widgets.h"

#include <Arduino.h>
#include <cmath>
#include <cstdlib>

// ── Icon bitmaps (include only in this TU to avoid duplicate symbols) ──
#include "assets/icons/icons_16x16.h"
#include "assets/icons/icons_24x24.h"
#include "assets/icons/icons_32x32.h"
#include "assets/icons/icons_96x96.h"

// ── Fonts ──
#include "assets/fonts/FreeSans.h"  // defines FONT_5pt8b … FONT_18pt8b etc.

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
static const uint16_t CLR_BLACK = 0x0000;
static const uint16_t CLR_WHITE = 0xFFFF;

enum Align { LEFT, CENTER, RIGHT };

// Return pixel width of the string with the currently selected font.
static int16_t strW(Adafruit_GFX *g, const String &s) {
    int16_t x1, y1; uint16_t w, h;
    g->getTextBounds(s.c_str(), 0, 0, &x1, &y1, &w, &h);
    return (int16_t)w;
}

// Draw string with horizontal alignment; y is the text baseline.
static void drawStr(Adafruit_GFX *g, int16_t x, int16_t y,
                    const String &s, Align align,
                    uint16_t color = CLR_BLACK) {
    if (s.isEmpty()) return;
    int16_t tx = x;
    if (align == CENTER || align == RIGHT) {
        int16_t w = strW(g, s);
        tx = (align == CENTER) ? x - w / 2 : x - w;
    }
    g->setTextColor(color);
    g->setCursor(tx, y);
    g->print(s);
}

// Extract "HH:MM" from ISO 8601 datetime string "YYYY-MM-DDTHH:MM" or "HH:MM"
static String parseHHMM(const String &iso) {
    int tIdx = iso.indexOf('T');
    if (tIdx >= 0 && (int)iso.length() >= tIdx + 6) {
        return iso.substring(tIdx + 1, tIdx + 6);
    }
    if (iso.length() >= 5) return iso.substring(0, 5);
    return iso;
}

// ─────────────────────────────────────────────────────────────────────────────
// WMO code → icon bitmap selection (96×96 and 32×32)
// ─────────────────────────────────────────────────────────────────────────────
static const uint8_t *wmo96(int code, bool day) {
    switch (code) {
    case 0: case 1:
        return day ? wi_day_sunny_96x96 : wi_night_clear_96x96;
    case 2:
        return day ? wi_day_sunny_overcast_96x96 : wi_night_alt_partly_cloudy_96x96;
    case 3:
        return wi_cloudy_96x96;
    case 45: case 48:
        return day ? wi_day_fog_96x96 : wi_night_fog_96x96;
    case 51: case 53: case 55:
        return day ? wi_day_sprinkle_96x96 : wi_night_alt_sprinkle_96x96;
    case 56: case 57:
        return day ? wi_day_rain_mix_96x96 : wi_night_alt_rain_mix_96x96;
    case 61: case 63: case 65:
        return day ? wi_day_rain_96x96 : wi_night_alt_rain_96x96;
    case 66: case 67:
        return day ? wi_day_rain_mix_96x96 : wi_night_alt_rain_mix_96x96;
    case 71: case 73: case 75:
        return day ? wi_day_snow_96x96 : wi_night_alt_snow_96x96;
    case 77:
        return wi_snow_96x96;
    case 80: case 81: case 82:
        return day ? wi_day_showers_96x96 : wi_night_alt_showers_96x96;
    case 85: case 86:
        return day ? wi_day_snow_96x96 : wi_night_alt_snow_96x96;
    case 95:
        return day ? wi_day_thunderstorm_96x96 : wi_night_alt_thunderstorm_96x96;
    case 96: case 99:
        return day ? wi_day_hail_96x96 : wi_night_alt_hail_96x96;
    default:
        return wi_na_96x96;
    }
}

static const uint8_t *wmo32(int code) {
    // Forecast always uses daytime icons
    switch (code) {
    case 0: case 1:             return wi_day_sunny_32x32;
    case 2:                     return wi_day_sunny_overcast_32x32;
    case 3:                     return wi_cloudy_32x32;
    case 45: case 48:           return wi_day_fog_32x32;
    case 51: case 53: case 55:  return wi_day_sprinkle_32x32;
    case 56: case 57:           return wi_day_rain_mix_32x32;
    case 61: case 63: case 65:  return wi_day_rain_32x32;
    case 66: case 67:           return wi_day_rain_mix_32x32;
    case 71: case 73: case 75:  return wi_day_snow_32x32;
    case 77:                    return wi_snow_32x32;
    case 80: case 81: case 82:  return wi_day_showers_32x32;
    case 85: case 86:           return wi_day_snow_32x32;
    case 95:                    return wi_day_thunderstorm_32x32;
    case 96: case 99:           return wi_day_hail_32x32;
    default:                    return wi_na_32x32;
    }
}

// // AQI category string (US AQI scale)
// static const char *aqiDesc(int aqi) {
//     if (aqi < 0)    return "N/A";
//     if (aqi <= 50)  return "Good";
//     if (aqi <= 100) return "Moderate";
//     if (aqi <= 150) return "Unhealthy (S)";
//     if (aqi <= 200) return "Unhealthy";
//     if (aqi <= 300) return "Very Unhealthy";
//     return "Hazardous";
// }

// ─────────────────────────────────────────────────────────────────────────────
// PageWeather400x300
// ─────────────────────────────────────────────────────────────────────────────
void PageWeather400x300::create(Adafruit_GFX &gfx,
                                uint16_t w, uint16_t h,
                                uint16_t colorAccent,
                                bool hasAccent) {
    _gfx         = &gfx;
    _w           = w;
    _h           = h;
    _colorAccent = colorAccent;
    _hasAccent   = hasAccent;
}

void PageWeather400x300::draw() {
    _gfx->fillScreen(CLR_WHITE);
    _gfx->setTextWrap(false);
    _gfx->setTextSize(1);

    _drawCurrentConditions();
    _drawDataRows();
    _drawForecast();
    _drawStatusBar();
}

// ─────────────────────────────────────────────────────────────────────────────
// _drawCurrentConditions
// Top-left: 96×96 weather icon, temperature, feels-like; top-right: city/date
// Reference: DISP_3C_E420 sections of drawCurrentConditions() + drawLocationDate()
// ─────────────────────────────────────────────────────────────────────────────
void PageWeather400x300::_drawCurrentConditions() {
    // ── Weather icon 96×96 at (0, 0) ──
    const uint8_t *icon = (_weather && _weather->valid)
        ? wmo96(_weather->current.weather_code, _weather->current.is_day)
        : wi_day_sunny_96x96;  // placeholder
    _gfx->drawBitmap(0, 0, icon, 96, 96, CLR_WHITE, CLR_BLACK);

    // ── Temperature ──
    // Reference: display.setFont(&FONT_18pt8b);
    //            drawString(96+30-10, 96/2+25/2, dataStr, CENTER);  → x=116, y=60
    String tempStr = "--";
    String tempUnit = "\260C";
    if (_weather && _weather->valid) {
        float tempVal = _weather->current.temperature;
        if (_cfg && _cfg->unitsTemp == "F") {
            tempVal = tempVal * 9.0f / 5.0f + 32.0f;
            tempUnit = "\260F";
        }
        tempStr = String(static_cast<int>(std::round(tempVal)));
    }
    _gfx->setFont(&FONT_18pt8b);
    drawStr(_gfx, 116, 60, tempStr, CENTER);

    // Unit superscript: drawString(getCursorX(), 96/2-25/2+10, unitStr, LEFT) → y=46
    _gfx->setFont(&FONT_7pt8b);
    drawStr(_gfx, _gfx->getCursorX(), 46, tempUnit, LEFT);

    // ── Feels-like ──
    // Reference: display.setFont(&FONT_6pt8b);
    //            drawString(96+60/2, 96/2+25/2+12/2+17/2, dataStr, CENTER) → x=126, y=74
    String feelsStr = "Feels Like --";
    if (_weather && _weather->valid) {
        float feelsVal = _weather->current.apparent_temperature;
        if (_cfg && _cfg->unitsTemp == "F") feelsVal = feelsVal * 9.0f / 5.0f + 32.0f;
        feelsStr = "Feels Like " + String(static_cast<int>(std::round(feelsVal))) + tempUnit;
    }
    _gfx->setFont(&FONT_6pt8b);
    drawStr(_gfx, 126, 74, feelsStr, CENTER);

    // ── City name (top-right, accent color) ──
    // Reference: display.setFont(&FONT_8pt8b);
    //            drawString(DISP_WIDTH-2, 12, city, RIGHT, ACCENT_COLOR)
    String city = (_cfg && _cfg->city.length()) ? _cfg->city : "---";
    _gfx->setFont(&FONT_8pt8b);
    drawStr(_gfx, DISP_W - 2, 12, city, RIGHT, CLR_BLACK);

    // ── Date string ──
    // Reference: display.setFont(&FONT_6pt8b);
    //            drawString(DISP_WIDTH-2, 15+2+8, date, RIGHT)  → y=25
    time_t now = time(nullptr);
    tm *ti = localtime(&now);
    char dateBuf[48] = {};
    const char *dfmt = (_cfg && _cfg->dateFormat.length())
        ? _cfg->dateFormat.c_str() : "%a, %b %e";
    strftime(dateBuf, sizeof(dateBuf), dfmt, ti);
    _gfx->setFont(&FONT_6pt8b);
    drawStr(_gfx, DISP_W - 2, 25, String(dateBuf), RIGHT);
}

// ─────────────────────────────────────────────────────────────────────────────
// _drawDataRows
// Left panel data rows (5 rows × 2 columns), y starting at 104, stride 30.
//
// Reference DISP_3C_E420:
//   Left col icons  x=0,   labels x=24, values x=24  (+ 12 for value baseline)
//   Right col icons x=85,  labels x=105, values x=109
//
// Row 0: Sunrise      | Sunset
// Row 1: Wind         | Humidity
// Row 2: UV Index     | Pressure
// Row 3: Air Quality  | Visibility
// Row 4: Indoor Temp  | Indoor Humidity
// ─────────────────────────────────────────────────────────────────────────────
void PageWeather400x300::_drawDataRows() {
    // ─── Icons ───────────────────────────────────────────────────────────────
    // Left column icons (x=0)
    const uint8_t *iconsL[5] = {
        wi_sunrise_24x24,
        wi_strong_wind_24x24,
        wi_horizon_alt_24x24,    // was UV Index, now Altitude
        air_filter_24x24,
        house_thermometer_24x24
    };
    // Right column icons (x=85 = 170/2)
    const uint8_t *iconsR[5] = {
        wi_sunset_24x24,
        wi_humidity_24x24,
        wi_barometer_24x24,
        visibility_icon_24x24,
        house_humidity_24x24
    };
    for (int i = 0; i < 5; ++i) {
        _gfx->drawBitmap(0,  DATA_ROW_BASE + DATA_ROW_H * i,
                         iconsL[i], 24, 24, CLR_WHITE, CLR_BLACK);
        _gfx->drawBitmap(75, DATA_ROW_BASE + DATA_ROW_H * i,
                         iconsR[i], 24, 24, CLR_WHITE, CLR_BLACK);
    }

    // ─── Small labels (5pt font) ─────────────────────────────────────────────
    // Reference: display.setFont(&FreeSans_5pt8b);
    //            drawString(24,    104+5+(24+6)*i, TXT_..., LEFT)
    //            drawString(170/2+20, 104+5+(24+6)*i, TXT_..., LEFT)
    _gfx->setFont(&FreeSans_5pt8b);
    const char *labelsL[5] = { "Sunrise",  "Wind",     "Altitude", "Air Quality", "Indoor" };
    const char *labelsR[5] = { "Sunset",   "Humidity", "Pressure", "Visibility",  "Indoor" };
    for (int i = 0; i < 5; ++i) {
        drawStr(_gfx, 24,  DATA_ROW_BASE + 5 + DATA_ROW_H * i, labelsL[i], LEFT);
        drawStr(_gfx, 99, DATA_ROW_BASE + 5 + DATA_ROW_H * i, labelsR[i], LEFT);
    }

    // ─── Values ──────────────────────────────────────────────────────────────
    // Reference value_y = 104 + 5 + (24+6)*i + 24/2  = DATA_ROW_BASE+17+DATA_ROW_H*i
    // Left values: x=24, Right values: x=109 (=170/2+24)
    const int VY_BASE = DATA_ROW_BASE + 17; // 121

    const bool hasWeather = (_weather && _weather->valid);
    const bool hasAqi     = (_aqi     && _aqi->valid);

    // ── Row 0: Sunrise | Sunset ──
    String sunriseStr = "--:--";
    String sunsetStr  = "--:--";
    if (hasWeather && _weather->daily.size() > 0) {
        sunriseStr = parseHHMM(_weather->daily[0].sunrise);
        sunsetStr  = parseHHMM(_weather->daily[0].sunset);
    }
    _gfx->setFont(&FreeSans_7pt8b);
    drawStr(_gfx, 24,  VY_BASE + DATA_ROW_H * 0, sunriseStr, LEFT);
    drawStr(_gfx, 99, VY_BASE + DATA_ROW_H * 0, sunsetStr,  LEFT);

    // ── Row 1: Wind | Humidity ──
    String windStr  = "--";
    String windUnit = " km/h";
    if (hasWeather) {
        float windVal = _weather->current.wind_speed;
        if (_cfg) {
            if (_cfg->unitsSpeed == "ms")  { windVal /= 3.6f;       windUnit = " m/s";  }
            if (_cfg->unitsSpeed == "mph") { windVal *= 0.621371f;   windUnit = " mph";  }
            if (_cfg->unitsSpeed == "kn")  { windVal *= 0.539957f;   windUnit = " kn";   }
        }
        windStr = String(static_cast<int>(std::round(windVal)));
    }
    _gfx->setFont(&FreeSans_7pt8b);
    drawStr(_gfx, 24, VY_BASE + DATA_ROW_H * 1, windStr, LEFT);
    _gfx->setFont(&FONT_5pt8b);
    drawStr(_gfx, _gfx->getCursorX(), VY_BASE + DATA_ROW_H * 1, windUnit, LEFT);

    String humStr = "--";
    if (hasWeather)
        humStr = String(static_cast<int>(std::round(_weather->current.humidity)));
    _gfx->setFont(&FreeSans_7pt8b);
    drawStr(_gfx, 99, VY_BASE + DATA_ROW_H * 1, humStr, LEFT);
    _gfx->setFont(&FONT_5pt8b);
    drawStr(_gfx, _gfx->getCursorX(), VY_BASE + DATA_ROW_H * 1, "%", LEFT);

    // ── Row 2: Altitude | Pressure ──
    _gfx->setFont(&FreeSans_7pt8b);
    if (hasWeather && !isnan(_weather->elevation)) {
        drawStr(_gfx, 24, VY_BASE + DATA_ROW_H * 2,
                String(static_cast<int>(std::round(_weather->elevation))), LEFT);
        _gfx->setFont(&FONT_5pt8b);
        drawStr(_gfx, _gfx->getCursorX(), VY_BASE + DATA_ROW_H * 2, " m", LEFT);
    } else {
        drawStr(_gfx, 24, VY_BASE + DATA_ROW_H * 2, "--", LEFT);
    }

    String presStr  = "--";
    String presUnit = " hPa";
    if (hasWeather) {
        float presVal = _weather->current.pressure;
        if (_cfg) {
            if (_cfg->unitsPres == "inHg") { presVal *= 0.02953f;   presUnit = " inHg"; }
            if (_cfg->unitsPres == "mmHg") { presVal *= 0.750064f;  presUnit = " mmHg"; }
        }
        if (_cfg && _cfg->unitsPres == "inHg")
            presStr = String(presVal, 2);
        else
            presStr = String(static_cast<int>(std::round(presVal)));
    }
    _gfx->setFont(&FONT_7pt8b);
    drawStr(_gfx, 99, VY_BASE + DATA_ROW_H * 2, presStr, LEFT);
    _gfx->setFont(&FONT_5pt8b);
    drawStr(_gfx, _gfx->getCursorX(), VY_BASE + DATA_ROW_H * 2, presUnit, LEFT);

    // ── Row 3: Air Quality | Visibility ──
    _gfx->setFont(&FreeSans_7pt8b);
    if (hasAqi) {
        String aqiStr = String(_aqi->us_aqi);
        drawStr(_gfx, 24, VY_BASE + DATA_ROW_H * 3, aqiStr, LEFT);
    } else {
        drawStr(_gfx, 24, VY_BASE + DATA_ROW_H * 3, "N/A", LEFT);
    }

    // Visibility — API returns metres; convert per unitsDist
    _gfx->setFont(&FreeSans_7pt8b);
    if (hasWeather) {
        float visKm = _weather->current.visibility / 1000.0f;
        bool useMi = (_cfg && _cfg->unitsDist == "mi");
        float visVal  = useMi ? visKm * 0.621371f : visKm;
        const char *visUnit = useMi ? " mi" : " km";
        drawStr(_gfx, 99, VY_BASE + DATA_ROW_H * 3,
                String(static_cast<int>(std::round(visVal))), LEFT);
        _gfx->setFont(&FONT_5pt8b);
        drawStr(_gfx, _gfx->getCursorX(), VY_BASE + DATA_ROW_H * 3, visUnit, LEFT);
    } else {
        drawStr(_gfx, 99, VY_BASE + DATA_ROW_H * 3, "N/A", LEFT);
    }

    // ── Row 4: Indoor Temp | Indoor Humidity (from AHT20 sensor) ──
    _gfx->setFont(&FreeSans_7pt8b);
    if (!isnan(_indoorTemp)) {
        float tVal = _indoorTemp;
        String tUnit = "\260C";
        if (_cfg && _cfg->unitsTemp == "F") { tVal = tVal * 9.0f / 5.0f + 32.0f; tUnit = "\260F"; }
        drawStr(_gfx, 24, VY_BASE + DATA_ROW_H * 4,
                String(static_cast<int>(std::round(tVal))) + tUnit, LEFT);
    } else {
        drawStr(_gfx, 24, VY_BASE + DATA_ROW_H * 4, "--", LEFT);
    }
    if (!isnan(_indoorHumi)) {
        drawStr(_gfx, 99, VY_BASE + DATA_ROW_H * 4,
                String(static_cast<int>(std::round(_indoorHumi))), LEFT);
        _gfx->setFont(&FONT_5pt8b);
        drawStr(_gfx, _gfx->getCursorX(), VY_BASE + DATA_ROW_H * 4, "%", LEFT);
    } else {
        drawStr(_gfx, 99, VY_BASE + DATA_ROW_H * 4, "--", LEFT);
    }

    // ── Row 5: UV Index (left) | PM2.5 (right) ──
    const int16_t ROW5_Y       = DATA_ROW_BASE + DATA_ROW_H * 5;  // 254
    const int16_t ROW5_LABEL_Y = ROW5_Y + 5;   // 259
    const int16_t ROW5_VALUE_Y = ROW5_Y + 17;  // 271
    // Left: UV Index
    _gfx->drawBitmap(0, ROW5_Y, wi_day_sunny_24x24, 24, 24, CLR_WHITE, CLR_BLACK);
    _gfx->setFont(&FreeSans_5pt8b);
    drawStr(_gfx, 24, ROW5_LABEL_Y, "UV Index", LEFT);
    _gfx->setFont(&FreeSans_7pt8b);
    if (hasWeather && !_weather->daily.empty()) {
        drawStr(_gfx, 24, ROW5_VALUE_Y,
                String(_weather->daily[0].uv_index_max, 1), LEFT);
    } else {
        drawStr(_gfx, 24, ROW5_VALUE_Y, "N/A", LEFT);
    }
    // Right: PM2.5 (μg/m³)
    _gfx->drawBitmap(75, ROW5_Y, wi_dust_24x24, 24, 24, CLR_WHITE, CLR_BLACK);
    _gfx->setFont(&FreeSans_5pt8b);
    drawStr(_gfx, 99, ROW5_LABEL_Y, "PM2.5", LEFT);
    _gfx->setFont(&FreeSans_7pt8b);
    if (hasAqi && !isnan(_aqi->pm2_5)) {
        drawStr(_gfx, 99, ROW5_VALUE_Y,
                String(static_cast<int>(std::round(_aqi->pm2_5))), LEFT);
        _gfx->setFont(&FONT_5pt8b);
        drawStr(_gfx, _gfx->getCursorX(), ROW5_VALUE_Y, " ug/m3", LEFT);
    } else {
        drawStr(_gfx, 99, ROW5_VALUE_Y, "--", LEFT);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// _drawForecast  (right panel: 5-day columns + hourly temp graph)
// Reference: DISP_3C_E420 sections of drawForecast() + drawOutlookGraph()
//
// 5 forecast columns at x = 178 + i*44 (i = 0..4), stride = 44
//   icon  y ≈ 47  (= 49 + 69/4 - 32/2 - 6/2)
//   day   y ≈ 42  (= 49 + 69/4 - 32/2 - 26/2 - 6/2 + 16/2)
//   hi/lo y ≈ 88  (= 49 + 69/4 + 38/2 - 6/2 + 12/2)
//   center offset within 44-px column: +15
// ─────────────────────────────────────────────────────────────────────────────
void PageWeather400x300::_drawForecast() {
    const bool hasWeather = (_weather && _weather->valid);
    const int  numDays    = hasWeather ? (int)_weather->daily.size() : 0;

    // ── 5 forecast columns ──
    for (int i = 0; i < 5; ++i) {
        int16_t x   = 178 + i * 44;   // 178, 222, 266, 310, 354
        int16_t cx  = x + 15;          // center of column

        // ── Icon 32×32 at (x, 47) ──
        const uint8_t *fc_icon = (i < numDays)
            ? wmo32(_weather->daily[i].weather_code)
            : wi_day_sunny_32x32;   // placeholder
        _gfx->drawBitmap(x, 47, fc_icon, 32, 32, CLR_WHITE, CLR_BLACK);

        // ── Day-of-week label at y=42 ──
        char dayBuf[8] = {};
        if (i < numDays && _weather->daily[i].date.length() >= 10) {
            struct tm fcTm = {};
            fcTm.tm_year = _weather->daily[i].date.substring(0, 4).toInt() - 1900;
            fcTm.tm_mon  = _weather->daily[i].date.substring(5, 7).toInt() - 1;
            fcTm.tm_mday = _weather->daily[i].date.substring(8, 10).toInt();
            mktime(&fcTm);
            strftime(dayBuf, sizeof(dayBuf), "%a", &fcTm);
        } else {
            // Placeholder day name based on current weekday + i
            time_t now = time(nullptr);
            tm *ti = localtime(&now);
            struct tm fakeTm = *ti;
            fakeTm.tm_mday += i;
            mktime(&fakeTm);
            strftime(dayBuf, sizeof(dayBuf), "%a", &fakeTm);
        }
        _gfx->setFont(&FONT_6pt8b);
        drawStr(_gfx, cx, 42, String(dayBuf), CENTER);

        // ── Hi | Lo at y=88 ──
        String hiStr = "--";
        String loStr = "--";
        if (i < numDays) {
            float hiC = _weather->daily[i].temp_max;
            float loC = _weather->daily[i].temp_min;
            if (_cfg && _cfg->unitsTemp == "F") {
                hiStr = String(static_cast<int>(std::round(hiC * 9.0f / 5.0f + 32.0f))) + "\260";
                loStr = String(static_cast<int>(std::round(loC * 9.0f / 5.0f + 32.0f))) + "\260";
            } else {
                hiStr = String(static_cast<int>(std::round(hiC)));
                loStr = String(static_cast<int>(std::round(loC)));
            }
        }
        _gfx->setFont(&FONT_5pt8b);
        drawStr(_gfx, cx,     88, "|",    CENTER);
        drawStr(_gfx, cx - 3, 88, hiStr,  RIGHT);
        drawStr(_gfx, cx + 3, 88, loStr,  LEFT);
    }

    // ── Hourly temperature graph ──
    _drawHourlyGraph();
}

// ─────────────────────────────────────────────────────────────────────────────
// _drawHourlyGraph  (temperature line graph in right panel lower area)
// Reference: DISP_3C_E420 section of drawOutlookGraph()
//   xPos0=175, xPos1=400, yPos0=108, yPos1=DISP_HEIGHT-46=254
//   5 major y-ticks; x-axis interval = ceil(n/8) hours; labels FONT_5pt8b
// ─────────────────────────────────────────────────────────────────────────────
void PageWeather400x300::_drawHourlyGraph() {
    const int16_t xPos0 = 176;
    const int16_t xPos1 = DISP_W - 18;   // leave room for % labels on right
    const int16_t yPos0 = 108;
    const int16_t yPos1 = DISP_H - 46;   // 254
    const int yMajorTicks = 5;

    // ── Draw x-axis ──
    _gfx->drawLine(xPos0, yPos1,     xPos1, yPos1,     CLR_BLACK);
    _gfx->drawLine(xPos0, yPos1 + 1, xPos1, yPos1 + 1, CLR_BLACK);

    const bool hasWeather = (_weather && _weather->valid && !_weather->hourly.empty());

    int n = hasWeather ? (int)_weather->hourly.size() : 0;
    float tMin = 15.0f, tMax = 30.0f;  // defaults for placeholder

    if (hasWeather) {
        tMin = _weather->hourly[0].temperature;
        tMax = _weather->hourly[0].temperature;
        for (int i = 1; i < n; ++i) {
            float t = _weather->hourly[i].temperature;
            tMin = std::min(tMin, t);
            tMax = std::max(tMax, t);
        }
        if (_cfg && _cfg->unitsTemp == "F") {
            tMin = tMin * 9.0f / 5.0f + 32.0f;
            tMax = tMax * 9.0f / 5.0f + 32.0f;
        }
    }

    // Round bounds to nearest 5°, ensuring enough ticks
    int yMajorStep = 5;
    int tempBoundMin = static_cast<int>(tMin - 1) - (((static_cast<int>(tMin - 1) % yMajorStep) + yMajorStep) % yMajorStep);
    int tempBoundMax = static_cast<int>(tMax + 1) + (yMajorStep - ((static_cast<int>(tMax + 1) % yMajorStep + yMajorStep) % yMajorStep));
    while ((tempBoundMax - tempBoundMin) / yMajorStep > yMajorTicks) {
        yMajorStep += 5;
        tempBoundMin = static_cast<int>(tMin - 1) - (((static_cast<int>(tMin - 1) % yMajorStep) + yMajorStep) % yMajorStep);
        tempBoundMax = static_cast<int>(tMax + 1) + (yMajorStep - ((static_cast<int>(tMax + 1) % yMajorStep + yMajorStep) % yMajorStep));
    }
    while ((tempBoundMax - tempBoundMin) / yMajorStep < yMajorTicks) {
        if ((tMin - tempBoundMin) <= (tempBoundMax - tMax))
            tempBoundMin -= yMajorStep;
        else
            tempBoundMax += yMajorStep;
    }
    if (tempBoundMax == tempBoundMin) tempBoundMax = tempBoundMin + yMajorStep * yMajorTicks;

    // ── Y-axis tick labels and dotted grid lines ──
    _gfx->setFont(&FONT_5pt8b);
    float yInterval = (float)(yPos1 - yPos0) / (float)yMajorTicks;
    float yPxPerUnit = (float)(yPos1 - yPos0) / (float)(tempBoundMax - tempBoundMin);

    for (int i = 0; i <= yMajorTicks; ++i) {
        int yTick = static_cast<int>(yPos0 + i * yInterval);
        int tempTick = tempBoundMax - i * yMajorStep;
        String tStr = String(tempTick) + "\260";
        drawStr(_gfx, xPos0 - 9, yTick + 2, tStr, RIGHT, CLR_BLACK);
        // Right Y-axis: precipitation probability %
        drawStr(_gfx, DISP_W - 1, yTick + 2, String(100 - i * 20) + "%", RIGHT, CLR_BLACK);
        if (i < yMajorTicks) {
            for (int px = xPos0; px <= xPos1; px += 3)
                _gfx->drawPixel(px, yTick + (yTick % 2), CLR_BLACK);
        }
    }

    if (!hasWeather) return;  // no data — skip temperature line

    float xInterval = (float)(xPos1 - xPos0) / (float)n;
    int xMaxTicks = 8;
    int hourInterval = static_cast<int>(std::ceil((float)n / (float)xMaxTicks));

    // ── Precipitation probability bars: only at X-axis tick positions ──
    {
        float yPxPerPct = (float)(yPos1 - yPos0) / 100.0f;
        int barW = std::max(4, static_cast<int>(xInterval * hourInterval * 0.6f));
        for (int i = 0; i < n; ++i) {
            if ((i % hourInterval) != 0) continue;
            int precip = _weather->hourly[i].precipitation_probability;
            if (precip <= 0) continue;
            int barH = static_cast<int>(std::round(precip * yPxPerPct));
            int xTick = static_cast<int>(std::round(xPos0 + (float)i * xInterval));
            int bx   = xTick - barW / 2;  // center bar on tick
            int by0  = yPos1 - barH;
            for (int dy = 0; dy < barH; ++dy) {
                int by = by0 + dy;
                for (int dx = 0; dx < barW; ++dx) {
                    if (((bx + dx) + by) % 2 == 0)
                        _gfx->drawPixel(bx + dx, by, CLR_BLACK);
                }
            }
        }
    }

    // ── Temperature line + x-axis tick marks and labels ──
    int prevPx = -1, prevPy = -1;
    for (int i = 0; i < n; ++i) {
        float t = _weather->hourly[i].temperature;
        if (_cfg && _cfg->unitsTemp == "F") t = t * 9.0f / 5.0f + 32.0f;

        int px = static_cast<int>(std::round(xPos0 + (float)i * xInterval + 0.5f * xInterval));
        int py = static_cast<int>(std::round(yPos1 - yPxPerUnit * (t - (float)tempBoundMin)));

        // Draw line segment
        if (prevPx >= 0) {
            _gfx->drawLine(prevPx, prevPy,     px, py,     CLR_BLACK);
            _gfx->drawLine(prevPx, prevPy + 1, px, py + 1, CLR_BLACK);
            _gfx->drawLine(prevPx - 1, prevPy, px - 1, py, CLR_BLACK);
        }
        prevPx = px; prevPy = py;

        // X-axis tick mark and label every hourInterval hours
        int xTick = static_cast<int>(xPos0 + (float)i * xInterval);
        if ((i % hourInterval) == 0) {
            _gfx->drawLine(xTick,     yPos1 + 1, xTick,     yPos1 + 4, CLR_BLACK);
            _gfx->drawLine(xTick + 1, yPos1 + 1, xTick + 1, yPos1 + 4, CLR_BLACK);
            // Extract hour string from ISO datetime
            String hStr = parseHHMM(_weather->hourly[i].time);
            hStr = hStr.substring(0, 2);  // "HH"
            _gfx->setFont(&FONT_5pt8b);
            drawStr(_gfx, xTick, yPos1 + 1 + 12 + 4 + 3, hStr, CENTER);
        }
    }

    // ── Final tick mark ──
    if ((n % hourInterval) == 0) {
        int xTick = static_cast<int>(std::round(xPos0 + (float)n * xInterval));
        _gfx->drawLine(xTick,     yPos1 + 1, xTick,     yPos1 + 4, CLR_BLACK);
        _gfx->drawLine(xTick + 1, yPos1 + 1, xTick + 1, yPos1 + 4, CLR_BLACK);

        // Draw the final hour label (e.g. after 07 show 10); right-align to avoid clipping.
        String firstH = parseHHMM(_weather->hourly[0].time);
        int finalHour = 0;
        if (firstH.length() >= 2) {
            finalHour = (firstH.substring(0, 2).toInt() + n) % 24;
        }
        char hBuf[4] = {};
        snprintf(hBuf, sizeof(hBuf), "%02d", finalHour);
        _gfx->setFont(&FONT_5pt8b);
        drawStr(_gfx, xTick - 1, yPos1 + 1 + 12 + 4 + 3, String(hBuf), RIGHT);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// _drawStatusBar  (bottom strip: refresh time)
// Reference: DISP_3C_E420 section of drawStatusBar()
//   Text baseline: DISP_HEIGHT-1-2 = 297
//   24×24 icons top: DISP_HEIGHT-1-21 = 278
// ─────────────────────────────────────────────────────────────────────────────
void PageWeather400x300::_drawStatusBar() {
    _gfx->setFont(&FONT_7pt8b);

    // ── Last refresh time (right-aligned) ──
    time_t now = time(nullptr);
    tm *ti = localtime(&now);
    char timeBuf[36] = {};
    strftime(timeBuf, sizeof(timeBuf), "Updated %Y/%m/%d %H:%M:%S", ti);
    drawStr(_gfx, DISP_W - 2, DISP_H - 1 - 2, String(timeBuf), RIGHT);

    // ── IP address (left-aligned, same font) ──
    String ipStr = _localIP.length() ? _localIP : "---.---.---.---";
    drawStr(_gfx, 2, DISP_H - 1 - 2, ipStr, LEFT);
}

