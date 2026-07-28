#include <unity.h>

#include "app/display/display_page_state.h"
#include "app/page/page_catalog.h"
#include "app/page/page_manager.h"
#include "app/page/page_catalog.cpp"
#include "app/page/page_manager.cpp"
#include "app/display/display_page_state.cpp"

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

PageSettings focusSettings() {
    PageSettings settings = defaultPageSettings();
    settings.enabledMask = pageMask(PageId::WeatherToday) | pageMask(PageId::FocusClock);
    settings.autoRotateMask = settings.enabledMask;
    settings.orderCount = 2;
    settings.order[0] = PageId::WeatherToday;
    settings.order[1] = PageId::FocusClock;
    return settings;
}
}  // namespace

void test_power_on_starts_at_legacy_weather_home_page_zero() {
    PageSettings settings = weatherAndOverviewSettings();

    const DisplayPageState page = selectStartupDisplayPage(
        sanitizeStoredDisplayPage(static_cast<int32_t>(PageId::Overview), settings),
        settings,
        false);

    TEST_ASSERT_TRUE(page.isHomeWeather());
    TEST_ASSERT_EQUAL_INT32(kStoredHomeWeatherPage, storedValueForDisplayPage(page));
}

void test_focus_active_reset_can_force_restore_persisted_focus_page() {
    PageSettings settings = focusSettings();

    const DisplayPageState page = selectStartupDisplayPage(
        sanitizeStoredDisplayPage(static_cast<int32_t>(PageId::FocusClock), settings),
        settings,
        false,
        true);

    TEST_ASSERT_FALSE(page.isHomeWeather());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::FocusClock),
                            static_cast<uint8_t>(page.managedPage()));
}

void test_deep_sleep_restores_home_page_zero() {
    PageSettings settings = weatherAndOverviewSettings();

    const DisplayPageState page = selectStartupDisplayPage(
        sanitizeStoredDisplayPage(kStoredHomeWeatherPage, settings),
        settings,
        true);

    TEST_ASSERT_TRUE(page.isHomeWeather());
}

void test_deep_sleep_restores_managed_weather_today_as_page_one() {
    PageSettings settings = weatherAndOverviewSettings();

    const DisplayPageState page = selectStartupDisplayPage(
        sanitizeStoredDisplayPage(static_cast<int32_t>(PageId::WeatherToday), settings),
        settings,
        true);

    TEST_ASSERT_FALSE(page.isHomeWeather());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeatherToday),
                            static_cast<uint8_t>(page.managedPage()));
}

void test_invalid_stored_page_falls_back_to_home_page_zero() {
    PageSettings settings = weatherAndOverviewSettings();

    TEST_ASSERT_TRUE(sanitizeStoredDisplayPage(99, settings).isHomeWeather());
    TEST_ASSERT_TRUE(sanitizeStoredDisplayPage(static_cast<int32_t>(PageId::TodayAgenda), settings).isHomeWeather());
}

void test_button_navigation_includes_page_zero_before_managed_pages() {
    PageManager manager(weatherAndOverviewSettings());
    DisplayPageState page = DisplayPageState::homeWeather();

    page = applyButtonDisplayPageAction(manager, page, ButtonAction::NextPage);
    TEST_ASSERT_FALSE(page.isHomeWeather());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeatherToday),
                            static_cast<uint8_t>(page.managedPage()));

    page = applyButtonDisplayPageAction(manager, page, ButtonAction::NextPage);
    TEST_ASSERT_FALSE(page.isHomeWeather());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(page.managedPage()));

    page = applyButtonDisplayPageAction(manager, page, ButtonAction::NextPage);
    TEST_ASSERT_TRUE(page.isHomeWeather());

    page = applyButtonDisplayPageAction(manager, page, ButtonAction::PreviousPage);
    TEST_ASSERT_FALSE(page.isHomeWeather());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(page.managedPage()));

    page = applyButtonDisplayPageAction(manager, page, ButtonAction::PreviousPage);
    TEST_ASSERT_FALSE(page.isHomeWeather());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeatherToday),
                            static_cast<uint8_t>(page.managedPage()));

    page = applyButtonDisplayPageAction(manager, page, ButtonAction::PreviousPage);
    TEST_ASSERT_TRUE(page.isHomeWeather());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_power_on_starts_at_legacy_weather_home_page_zero);
    RUN_TEST(test_focus_active_reset_can_force_restore_persisted_focus_page);
    RUN_TEST(test_deep_sleep_restores_home_page_zero);
    RUN_TEST(test_deep_sleep_restores_managed_weather_today_as_page_one);
    RUN_TEST(test_invalid_stored_page_falls_back_to_home_page_zero);
    RUN_TEST(test_button_navigation_includes_page_zero_before_managed_pages);
    UNITY_END();
}

void loop() {}
