#include <unity.h>

#include "app/render/render_coordinator.h"
#include "app/page/page_catalog.cpp"
#include "app/render/render_coordinator.cpp"

void test_busy_display_defers_latest_target_page() {
    RenderCoordinator coordinator;
    RenderInputs input;
    input.epdBusy = true;
    input.requestedPage = PageId::Headlines;
    input.currentPage = PageId::WeatherToday;
    input.contentHash = 42;
    input.lastContentHash = 1;
    input.forceRefresh = true;

    const RenderDecision decision = coordinator.decide(input);

    TEST_ASSERT_FALSE(decision.shouldRender);
    TEST_ASSERT_TRUE(decision.deferred);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Headlines),
                            static_cast<uint8_t>(decision.targetPage));
}

void test_same_page_same_hash_skips_refresh() {
    RenderCoordinator coordinator;
    RenderInputs input;
    input.requestedPage = PageId::WeatherToday;
    input.currentPage = PageId::WeatherToday;
    input.contentHash = 1234;
    input.lastContentHash = 1234;
    input.lastFullRefreshUtc = 100;
    input.nowUtc = 120;

    const RenderDecision decision = coordinator.decide(input);

    TEST_ASSERT_FALSE(decision.shouldRender);
    TEST_ASSERT_FALSE(decision.deferred);
}

void test_refresh_is_throttled_inside_minimum_interval() {
    RenderCoordinator coordinator;
    RenderInputs input;
    input.requestedPage = PageId::WeatherToday;
    input.currentPage = PageId::WeatherToday;
    input.contentHash = 1235;
    input.lastContentHash = 1234;
    input.lastFullRefreshUtc = 100;
    input.nowUtc = 105;

    const RenderDecision decision = coordinator.decide(input);

    TEST_ASSERT_FALSE(decision.shouldRender);
    TEST_ASSERT_TRUE(decision.deferred);
}

void test_changed_hash_after_interval_renders() {
    RenderCoordinator coordinator;
    RenderInputs input;
    input.requestedPage = PageId::WeatherToday;
    input.currentPage = PageId::WeatherToday;
    input.contentHash = 1235;
    input.lastContentHash = 1234;
    input.lastFullRefreshUtc = 100;
    input.nowUtc = 111;

    const RenderDecision decision = coordinator.decide(input);

    TEST_ASSERT_TRUE(decision.shouldRender);
    TEST_ASSERT_FALSE(decision.deferred);
}

void test_force_refresh_renders_even_when_hash_matches() {
    RenderCoordinator coordinator;
    RenderInputs input;
    input.requestedPage = PageId::PortfolioSummary;
    input.currentPage = PageId::PortfolioSummary;
    input.contentHash = 77;
    input.lastContentHash = 77;
    input.lastFullRefreshUtc = 100;
    input.nowUtc = 120;
    input.forceRefresh = true;

    const RenderDecision decision = coordinator.decide(input);

    TEST_ASSERT_TRUE(decision.shouldRender);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_busy_display_defers_latest_target_page);
    RUN_TEST(test_same_page_same_hash_skips_refresh);
    RUN_TEST(test_refresh_is_throttled_inside_minimum_interval);
    RUN_TEST(test_changed_hash_after_interval_renders);
    RUN_TEST(test_force_refresh_renders_even_when_hash_matches);
    UNITY_END();
}

void loop() {}
