#include <unity.h>

#include <cmath>

#include "app/weather/home_weather_snapshot.h"
#include "app/weather/home_weather_snapshot.cpp"

namespace {
WeatherData chengduWeather() {
    WeatherData weather;
    weather.valid = true;
    weather.current.time = "2026-09-15T10:00";
    weather.current.temperature = 21.0f;
    weather.current.weather_code = 2;
    weather.current.cloud_cover = 62.0f;
    weather.current.precipitation = 0.0f;
    weather.current.wind_speed = 12.96f;
    weather.current.is_day = true;
    weather.daily.push_back(WeatherDaily{"2026-09-15", 2, 26.0f, 18.0f, 5.0f,
                                         "2026-09-15T06:40", "2026-09-15T19:05"});
    for (int hour = 10; hour <= 21; ++hour) {
        char timestamp[20] = {};
        std::snprintf(timestamp, sizeof(timestamp), "2026-09-15T%02d:00", hour);
        WeatherHourly item;
        item.time = timestamp;
        item.temperature = 21.0f + static_cast<float>((hour - 10) % 6);
        item.weather_code = hour >= 18 ? 61 : 2;
        item.precipitation_probability = hour >= 18 ? 80 : 0;
        item.precipitation = hour >= 18 ? 0.3f : 0.0f;
        item.humidity = 70;
        weather.hourly.push_back(item);
    }
    return weather;
}

AppConfig metricConfig() {
    AppConfig config;
    config.city = "Chengdu, Sichuan, China";
    config.unitsTemp = "C";
    config.unitsSpeed = "ms";
    return config;
}
}  // namespace

void test_snapshot_uses_forward_hourly_rain_and_normalized_wind() {
    const HomeWeatherSnapshot snapshot = buildHomeWeatherSnapshot(chengduWeather(), metricConfig());

    TEST_ASSERT_TRUE(snapshot.valid);
    TEST_ASSERT_EQUAL_STRING("CHENGDU", snapshot.location.c_str());
    TEST_ASSERT_EQUAL_STRING("PARTLY CLOUDY", snapshot.condition.c_str());
    TEST_ASSERT_EQUAL_FLOAT(21.0f, snapshot.currentTempC);
    TEST_ASSERT_EQUAL_FLOAT(18.0f, snapshot.lowTempC);
    TEST_ASSERT_EQUAL_FLOAT(26.0f, snapshot.highTempC);
    TEST_ASSERT_EQUAL_FLOAT(62.0f, snapshot.cloudCoverPct);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, snapshot.currentPrecipitationMm);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.6f, snapshot.windMps);
    TEST_ASSERT_EQUAL_UINT(12, snapshot.hourly.size());
    TEST_ASSERT_EQUAL_STRING("RAIN FROM 18:00", homeRainOutlook(snapshot).c_str());
}

void test_snapshot_does_not_treat_future_rain_as_current_rain() {
    const HomeWeatherSnapshot snapshot = buildHomeWeatherSnapshot(chengduWeather(), metricConfig());

    TEST_ASSERT_FALSE(homeCurrentRainActive(snapshot));
    TEST_ASSERT_TRUE(homeHourlyRainActive(snapshot.hourly[8]));
}

void test_snapshot_reports_missing_hourly_precipitation() {
    WeatherData weather = chengduWeather();
    for (WeatherHourly &hour : weather.hourly) {
        hour.precipitation = NAN;
    }

    const HomeWeatherSnapshot snapshot = buildHomeWeatherSnapshot(weather, metricConfig());

    TEST_ASSERT_EQUAL_STRING("RAIN DATA --", homeRainOutlook(snapshot).c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_snapshot_uses_forward_hourly_rain_and_normalized_wind);
    RUN_TEST(test_snapshot_does_not_treat_future_rain_as_current_rain);
    RUN_TEST(test_snapshot_reports_missing_hourly_precipitation);
    UNITY_END();
}

void loop() {}
