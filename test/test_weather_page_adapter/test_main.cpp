#include <unity.h>

#include "app/weather/weather_page_adapter.h"
#include "app/weather/weather_page_adapter.cpp"

void test_weather_fallback_uses_current_date_window_and_configured_fahrenheit() {
    const int64_t localEpoch = 1785142800LL;  // 2026-07-27 09:00 local epoch.

    const WeatherPageSnapshot snapshot =
        weatherFallbackPageSnapshot("NEW YORK", "NY", "USA", "F", localEpoch);

    TEST_ASSERT_EQUAL_STRING("F", snapshot.tempUnit.c_str());
    TEST_ASSERT_EQUAL_UINT32(7, snapshot.weekly.size());
    TEST_ASSERT_EQUAL_STRING("MON", snapshot.weekly[0].label.c_str());
    TEST_ASSERT_EQUAL_STRING("JUL 27", snapshot.weekly[0].dateLabel.c_str());
    TEST_ASSERT_TRUE(snapshot.weekly[0].today);
    TEST_ASSERT_EQUAL_STRING("SUN", snapshot.weekly[6].label.c_str());
    TEST_ASSERT_EQUAL_STRING("AUG 02", snapshot.weekly[6].dateLabel.c_str());
}

void test_weather_fallback_keeps_configured_celsius_unit() {
    const int64_t localEpoch = 1785142800LL;  // 2026-07-27 09:00 local epoch.

    const WeatherPageSnapshot snapshot =
        weatherFallbackPageSnapshot("CHENGDU", "SICHUAN", "CHINA", "C", localEpoch);

    TEST_ASSERT_EQUAL_STRING("C", snapshot.tempUnit.c_str());
    TEST_ASSERT_EQUAL_STRING("CHENGDU", snapshot.city.c_str());
    TEST_ASSERT_EQUAL_STRING("SICHUAN", snapshot.region.c_str());
    TEST_ASSERT_EQUAL_STRING("CHINA", snapshot.country.c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_weather_fallback_uses_current_date_window_and_configured_fahrenheit);
    RUN_TEST(test_weather_fallback_keeps_configured_celsius_unit);
    UNITY_END();
}

void loop() {}
