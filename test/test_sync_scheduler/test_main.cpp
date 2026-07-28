#include <unity.h>

#include "app/page/page_catalog.cpp"
#include "app/scheduler/sync_scheduler.h"
#include "app/scheduler/sync_scheduler.cpp"

void test_unreferenced_provider_is_not_selected() {
    SyncScheduler scheduler;
    const std::vector<ProviderSyncState> states = {
        {"calendar", 300, 0, 0, true},
        {"finance", 300, 0, 0, true},
    };

    const auto due = scheduler.selectDueProviders(states, {"calendar"}, 1000);

    TEST_ASSERT_EQUAL_UINT(1, due.size());
    TEST_ASSERT_EQUAL_STRING("calendar", due[0].c_str());
}

void test_retry_after_blocks_rate_limited_provider() {
    SyncScheduler scheduler;
    const std::vector<ProviderSyncState> states = {
        {"calendar", 300, 900, 1300, true},
    };

    const auto beforeRetry = scheduler.selectDueProviders(states, {"calendar"}, 1200);
    const auto afterRetry = scheduler.selectDueProviders(states, {"calendar"}, 1300);

    TEST_ASSERT_TRUE(beforeRetry.empty());
    TEST_ASSERT_EQUAL_UINT(1, afterRetry.size());
    TEST_ASSERT_EQUAL_STRING("calendar", afterRetry[0].c_str());
}

void test_ttl_controls_due_provider_selection() {
    SyncScheduler scheduler;
    const std::vector<ProviderSyncState> states = {
        {"calendar", 300, 1000, 0, true},
        {"weather", 300, 600, 0, true},
    };

    const auto due = scheduler.selectDueProviders(states, {"calendar", "weather"}, 1000);

    TEST_ASSERT_EQUAL_UINT(1, due.size());
    TEST_ASSERT_EQUAL_STRING("weather", due[0].c_str());
}

void test_display_page_sync_requirements_match_visible_page() {
    auto req = syncRequirementsForDisplayPage(/*homeWeather=*/true, PageId::FocusClock);
    TEST_ASSERT_TRUE(req.weather);
    TEST_ASSERT_FALSE(req.calendar);
    TEST_ASSERT_FALSE(req.finance);
    TEST_ASSERT_FALSE(req.news);

    req = syncRequirementsForDisplayPage(false, PageId::FocusClock);
    TEST_ASSERT_FALSE(req.weather);
    TEST_ASSERT_FALSE(req.calendar);
    TEST_ASSERT_FALSE(req.finance);
    TEST_ASSERT_FALSE(req.news);

    req = syncRequirementsForDisplayPage(false, PageId::WorldClock);
    TEST_ASSERT_FALSE(req.weather);
    TEST_ASSERT_FALSE(req.calendar);
    TEST_ASSERT_FALSE(req.finance);
    TEST_ASSERT_FALSE(req.news);

    req = syncRequirementsForDisplayPage(false, PageId::Overview);
    TEST_ASSERT_TRUE(req.weather);
    TEST_ASSERT_TRUE(req.calendar);
    TEST_ASSERT_FALSE(req.finance);
    TEST_ASSERT_FALSE(req.news);

    req = syncRequirementsForDisplayPage(false, PageId::StockInfo);
    TEST_ASSERT_FALSE(req.weather);
    TEST_ASSERT_FALSE(req.calendar);
    TEST_ASSERT_TRUE(req.finance);
    TEST_ASSERT_FALSE(req.news);

    req = syncRequirementsForDisplayPage(false, PageId::Headlines);
    TEST_ASSERT_FALSE(req.weather);
    TEST_ASSERT_FALSE(req.calendar);
    TEST_ASSERT_FALSE(req.finance);
    TEST_ASSERT_TRUE(req.news);
}

void test_missing_sync_requirements_detects_calendar_after_weather_home() {
    PageSyncRequirements completed;
    completed.weather = true;
    const PageSyncRequirements agenda = syncRequirementsForDisplayPage(false, PageId::TodayAgenda);

    const PageSyncRequirements missing = missingSyncRequirements(agenda, completed);

    TEST_ASSERT_FALSE(missing.weather);
    TEST_ASSERT_TRUE(missing.calendar);
    TEST_ASSERT_FALSE(missing.finance);
    TEST_ASSERT_FALSE(missing.news);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_unreferenced_provider_is_not_selected);
    RUN_TEST(test_retry_after_blocks_rate_limited_provider);
    RUN_TEST(test_ttl_controls_due_provider_selection);
    RUN_TEST(test_display_page_sync_requirements_match_visible_page);
    RUN_TEST(test_missing_sync_requirements_detects_calendar_after_weather_home);
    UNITY_END();
}

void loop() {}
