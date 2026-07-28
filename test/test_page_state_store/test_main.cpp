#include <unity.h>

#include "app/page/page_catalog.h"
#include "app/page/page_manager.h"
#include "app/page/page_state_store.h"
#include "app/page/page_catalog.cpp"
#include "app/page/page_manager.cpp"
#include "app/page/page_state_store.cpp"

namespace {
PageSettings weatherAndOverviewSettings() {
    PageSettings settings = defaultPageSettings();
    settings.enabledMask = pageMask(PageId::WeatherToday) | pageMask(PageId::Overview);
    settings.autoRotateMask = settings.enabledMask;
    settings.orderCount = 2;
    settings.order[0] = PageId::WeatherToday;
    settings.order[1] = PageId::Overview;
    return settings;
}
}  // namespace

void test_absent_page_defaults_to_weather_home() {
    PageSettings settings = weatherAndOverviewSettings();

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeatherToday),
                            static_cast<uint8_t>(sanitizeStoredPageId(-1, settings)));
}

void test_valid_enabled_page_is_restored() {
    PageSettings settings = weatherAndOverviewSettings();

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(sanitizeStoredPageId(static_cast<int32_t>(PageId::Overview), settings)));
}

void test_power_on_startup_uses_weather_home_even_with_stored_page() {
    PageSettings settings = weatherAndOverviewSettings();

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeatherToday),
                            static_cast<uint8_t>(selectStartupPage(PageId::Overview, settings, false)));
}

void test_deep_sleep_startup_restores_stored_page() {
    PageSettings settings = weatherAndOverviewSettings();

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(selectStartupPage(PageId::Overview, settings, true)));
}

void test_invalid_or_disabled_page_falls_back_to_weather_home() {
    PageSettings settings = weatherAndOverviewSettings();

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeatherToday),
                            static_cast<uint8_t>(sanitizeStoredPageId(99, settings)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeatherToday),
                            static_cast<uint8_t>(sanitizeStoredPageId(static_cast<int32_t>(PageId::TodayAgenda), settings)));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_absent_page_defaults_to_weather_home);
    RUN_TEST(test_valid_enabled_page_is_restored);
    RUN_TEST(test_power_on_startup_uses_weather_home_even_with_stored_page);
    RUN_TEST(test_deep_sleep_startup_restores_stored_page);
    RUN_TEST(test_invalid_or_disabled_page_falls_back_to_weather_home);
    UNITY_END();
}

void loop() {}
