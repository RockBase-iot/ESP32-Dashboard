#include "weather.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "utils/logger.h"

static const char *TAG = "Weather";

bool WeatherClass::fetchWeather(double lat, double lon) {
    // Build Open-Meteo request URL.
    // Fixed SI units; the UI layer converts to user-selected units.
    char url[512];
    snprintf(url, sizeof(url),
        "http://" WEATHER_API_HOST "/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,apparent_temperature,relative_humidity_2m,"
                  "wind_speed_10m,wind_direction_10m,surface_pressure,"
                  "weather_code,is_day,visibility"
        "&hourly=temperature_2m,weather_code,precipitation_probability,relative_humidity_2m"
        "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max"
        "&forecast_days=5&forecast_hours=24&timezone=auto",
        lat, lon);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(WEATHER_HTTP_TIMEOUT);
    int code = http.GET();
    if (code != 200) {
        log_e(TAG, "HTTP %d for weather request", code);
        http.end();
        return false;
    }

    String body = http.getString();
    // log_i(TAG, "=== Weather raw response (%d bytes) ===", (int)body.length());
    // // Print in 200-char chunks to avoid serial truncation
    // for (int off = 0; off < (int)body.length(); off += 200) {
    //     log_i(TAG, "%s", body.substring(off, off + 200).c_str());
    // }
    bool ok = _parseWeatherResponse(body);
    http.end();
    return ok;
}

bool WeatherClass::fetchAirQuality(double lat, double lon) {
    char url[512];
    snprintf(url, sizeof(url),
        "http://" AQI_API_HOST "/v1/air-quality"
        "?latitude=%.4f&longitude=%.4f"
        "&current=pm2_5,pm10,us_aqi,european_aqi"
        "&hourly=us_aqi,pm2_5,pm10"
        "&forecast_days=4&timezone=auto",
        lat, lon);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(WEATHER_HTTP_TIMEOUT);
    int code = http.GET();
    if (code != 200) {
        log_e(TAG, "HTTP %d for AQI request", code);
        http.end();
        return false;
    }

    bool ok = _parseAqiResponse(http.getString());
    http.end();
    return ok;
}

bool WeatherClass::_parseWeatherResponse(const String &body) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        log_e(TAG, "JSON parse error: %s", err.c_str());
        return false;
    }

    // ── Current ──────────────────────────────────────────────────────────
    JsonObject cur = doc["current"];
    _weather.current.temperature          = cur["temperature_2m"];
    _weather.current.apparent_temperature = cur["apparent_temperature"];
    _weather.current.humidity             = cur["relative_humidity_2m"];
    _weather.current.wind_speed           = cur["wind_speed_10m"];
    _weather.current.wind_direction       = cur["wind_direction_10m"];
    _weather.current.pressure             = cur["surface_pressure"];
    _weather.current.visibility           = cur["visibility"];
    _weather.current.weather_code         = cur["weather_code"];
    _weather.current.is_day               = (bool)cur["is_day"];
    _weather.timezone  = doc["timezone"].as<String>();
    _weather.elevation = doc["elevation"].as<float>();

    // ── Daily (5 days) ───────────────────────────────────────────────────
    _weather.daily.clear();
    JsonObject daily     = doc["daily"];
    JsonArray  dTime     = daily["time"];
    JsonArray  dCode     = daily["weather_code"];
    JsonArray  dTempMax  = daily["temperature_2m_max"];
    JsonArray  dTempMin  = daily["temperature_2m_min"];
    JsonArray  dSunrise  = daily["sunrise"];
    JsonArray  dSunset   = daily["sunset"];
    JsonArray  dUvIndex  = daily["uv_index_max"];
    size_t numDays = dTime.size();
    _weather.daily.reserve(numDays);
    for (size_t i = 0; i < numDays; ++i) {
        WeatherDaily d;
        d.date          = dTime[i].as<String>();
        d.weather_code  = dCode[i].as<int>();
        d.temp_max      = dTempMax[i].as<float>();
        d.temp_min      = dTempMin[i].as<float>();
        d.uv_index_max  = dUvIndex[i].as<float>();
        d.sunrise       = dSunrise[i].as<String>();
        d.sunset        = dSunset[i].as<String>();
        _weather.daily.push_back(d);
    }

    // ── Hourly (first 24 entries) ────────────────────────────────────────
    _weather.hourly.clear();
    JsonObject hourly = doc["hourly"];
    JsonArray  hTime  = hourly["time"];
    JsonArray  hTemp  = hourly["temperature_2m"];
    JsonArray  hCode  = hourly["weather_code"];
    JsonArray  hPrec  = hourly["precipitation_probability"];
    JsonArray  hHumi  = hourly["relative_humidity_2m"];
    size_t numHours = hTime.size();
    if (numHours > 24) numHours = 24;
    _weather.hourly.reserve(numHours);
    for (size_t i = 0; i < numHours; ++i) {
        WeatherHourly h;
        h.time                     = hTime[i].as<String>();
        h.temperature              = hTemp[i].as<float>();
        h.weather_code             = hCode[i].as<int>();
        h.precipitation_probability = hPrec[i].as<int>();
        h.humidity                 = hHumi[i].as<int>();
        _weather.hourly.push_back(h);
    }

    _weather.last_update_ms = millis();
    _weather.valid = true;

    // // ── Print summary ──────────────────────────────────────────────────────
    // const auto &c = _weather.current;
    // log_i(TAG, "--- Weather current ---");
    // log_i(TAG, "  Timezone : %s", _weather.timezone.c_str());
    // log_i(TAG, "  Condition: WMO %d  is_day=%d", c.weather_code, (int)c.is_day);
    // log_i(TAG, "  Temp     : %.1f C  feels %.1f C", c.temperature, c.apparent_temperature);
    // log_i(TAG, "  Humidity : %.0f%%", c.humidity);
    // log_i(TAG, "  Wind     : %.1f km/h  dir %.0f deg", c.wind_speed, c.wind_direction);
    // log_i(TAG, "  Pressure : %.1f hPa", c.pressure);
    // // ── Daily detail ──────────────────────────────────────────────────────
    // log_i(TAG, "--- Daily forecast (%d days) ---", (int)_weather.daily.size());
    // for (size_t i = 0; i < _weather.daily.size(); ++i) {
    //     const auto &d = _weather.daily[i];
    //     log_i(TAG, "  [%d] %s  WMO=%d  max=%.1fC min=%.1fC  UV=%.1f  sunrise=%s sunset=%s",
    //           (int)i, d.date.c_str(), d.weather_code,
    //           d.temp_max, d.temp_min, d.uv_index_max,
    //           d.sunrise.c_str(), d.sunset.c_str());
    // }

    // // ── Hourly detail ────────────────────────────────────────────────────
    // log_i(TAG, "--- Hourly forecast (%d hours) ---", (int)_weather.hourly.size());
    // for (size_t i = 0; i < _weather.hourly.size(); ++i) {
    //     const auto &h = _weather.hourly[i];
    //     log_i(TAG, "  [%02d] %s  temp=%.1fC  humi=%d%%  precip=%d%%  WMO=%d",
    //           (int)i, h.time.c_str(), h.temperature,
    //           h.humidity, h.precipitation_probability, h.weather_code);
    // }
    return true;
}

bool WeatherClass::_parseAqiResponse(const String &body) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        log_e(TAG, "AQI JSON parse error: %s", err.c_str());
        return false;
    }
    JsonObject cur = doc["current"];
    _aqi.pm2_5         = cur["pm2_5"];
    _aqi.pm10          = cur["pm10"];
    _aqi.us_aqi        = cur["us_aqi"];
    _aqi.european_aqi  = cur["european_aqi"];

    // TODO: parse daily AQI arrays.

    _aqi.last_update_ms = millis();
    _aqi.valid = true;

    // // ── Print full AQI data ────────────────────────────────────────────────
    // log_i(TAG, "--- AQI current ---");
    // log_i(TAG, "  PM2.5        : %.1f ug/m3", _aqi.pm2_5);
    // log_i(TAG, "  PM10         : %.1f ug/m3", _aqi.pm10);
    // log_i(TAG, "  US AQI       : %d", _aqi.us_aqi);
    // log_i(TAG, "  European AQI : %d", _aqi.european_aqi);
    // log_i(TAG, "--- AQI raw hourly (us_aqi / pm2_5 / pm10) ---");
    // {
    //     JsonObject hourly = doc["hourly"];
    //     JsonArray  hTime  = hourly["time"];
    //     JsonArray  hUsAqi = hourly["us_aqi"];
    //     JsonArray  hPm25  = hourly["pm2_5"];
    //     JsonArray  hPm10  = hourly["pm10"];
    //     size_t nh = hTime.size();
    //     for (size_t i = 0; i < nh; ++i) {
    //         log_i(TAG, "  [%02d] %s  us_aqi=%d  pm2.5=%.1f  pm10=%.1f",
    //               (int)i, hTime[i].as<String>().c_str(),
    //               hUsAqi[i].as<int>(),
    //               hPm25[i].as<float>(),
    //               hPm10[i].as<float>());
    //     }
    // }
    return true;
}
