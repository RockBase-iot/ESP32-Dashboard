#include <unity.h>

#include <cmath>

#include "app/weather/weather.h"
#include "app/weather/weather.cpp"

void test_open_meteo_parser_keeps_home_weather_fields() {
    const String body = R"json({
        "timezone":"Asia/Shanghai",
        "elevation":500,
        "current":{
            "time":"2026-09-15T10:00",
            "temperature_2m":21,
            "apparent_temperature":21,
            "relative_humidity_2m":70,
            "wind_speed_10m":12.96,
            "wind_direction_10m":90,
            "surface_pressure":950,
            "weather_code":2,
            "is_day":1,
            "visibility":10000,
            "cloud_cover":62,
            "precipitation":0
        },
        "hourly":{
            "time":["2026-09-15T10:00","2026-09-15T18:00"],
            "temperature_2m":[21,22],
            "weather_code":[2,61],
            "precipitation_probability":[0,80],
            "precipitation":[0,0.3],
            "relative_humidity_2m":[70,85]
        },
        "daily":{
            "time":["2026-09-15"],
            "weather_code":[2],
            "temperature_2m_max":[26],
            "temperature_2m_min":[18],
            "sunrise":["2026-09-15T06:40"],
            "sunset":["2026-09-15T19:05"],
            "uv_index_max":[5]
        }
    })json";
    WeatherData weather;

    TEST_ASSERT_TRUE(parseWeatherResponseBody(body, weather));
    TEST_ASSERT_EQUAL_STRING("2026-09-15T10:00", weather.current.time.c_str());
    TEST_ASSERT_EQUAL_FLOAT(62.0f, weather.current.cloud_cover);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, weather.current.precipitation);
    TEST_ASSERT_EQUAL_UINT(2, weather.hourly.size());
    TEST_ASSERT_EQUAL_FLOAT(0.3f, weather.hourly[1].precipitation);
}

void test_missing_precipitation_is_not_treated_as_zero() {
    const String body = R"json({
        "current":{"time":"2026-09-15T10:00","temperature_2m":21},
        "hourly":{"time":[],"temperature_2m":[],"weather_code":[],"precipitation_probability":[],"relative_humidity_2m":[]},
        "daily":{"time":[],"weather_code":[],"temperature_2m_max":[],"temperature_2m_min":[],"sunrise":[],"sunset":[],"uv_index_max":[]}
    })json";
    WeatherData weather;

    TEST_ASSERT_TRUE(parseWeatherResponseBody(body, weather));
    TEST_ASSERT_TRUE(std::isnan(weather.current.precipitation));
    TEST_ASSERT_TRUE(std::isnan(weather.current.cloud_cover));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_open_meteo_parser_keeps_home_weather_fields);
    RUN_TEST(test_missing_precipitation_is_not_treated_as_zero);
    UNITY_END();
}

void loop() {}
