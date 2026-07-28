#include <unity.h>

#include "app/config/app_config.h"
#include "app/config/config_health.h"
#include "app/page/page_catalog.cpp"
#include "app/page/page_manager.cpp"
#include "app/config/config_health.cpp"

namespace {
AppConfig baseConfig() {
    AppConfig cfg;
    cfg.lat = "30.6667";
    cfg.lon = "104.0667";
    cfg.city = "Chengdu, Sichuan, China";
    return cfg;
}

PageSettings pages(std::initializer_list<PageId> enabled) {
    PageSettings settings = defaultPageSettings();
    settings.enabledMask = 0;
    settings.autoRotateMask = 0;
    settings.orderCount = 0;
    for (PageId page : enabled) {
        settings.enabledMask |= pageMask(page);
        settings.autoRotateMask |= pageMask(page);
        settings.order[settings.orderCount++] = page;
    }
    return sanitizePageSettings(settings);
}
}  // namespace

void test_weather_page_is_ready_when_location_is_configured() {
    SourceConfigSummary sources;
    const ConfigHealth health = buildConfigHealth(
        baseConfig(), pages({PageId::WeatherToday}), sources);

    const PageReadiness *weather = findPageReadiness(health, PageId::WeatherToday);
    TEST_ASSERT_NOT_NULL(weather);
    TEST_ASSERT_TRUE(weather->enabled);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Ready),
                            static_cast<uint8_t>(weather->state));
    TEST_ASSERT_EQUAL_UINT(2, health.totalDisplayPages);
    TEST_ASSERT_TRUE(health.allEnabledPagesReady);
}

void test_calendar_page_requires_calendar_source_when_enabled() {
    SourceConfigSummary sources;
    const ConfigHealth health = buildConfigHealth(
        baseConfig(), pages({PageId::TodayAgenda}), sources);

    const PageReadiness *agenda = findPageReadiness(health, PageId::TodayAgenda);
    TEST_ASSERT_NOT_NULL(agenda);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Required),
                            static_cast<uint8_t>(agenda->state));
    TEST_ASSERT_FALSE(health.allEnabledPagesReady);
    TEST_ASSERT_EQUAL_UINT(1, health.requiredCount);
}

void test_calendar_page_becomes_ready_with_enabled_calendar_source() {
    SourceConfigSummary sources;
    sources.enabledCalendarSources = 1;
    const ConfigHealth health = buildConfigHealth(
        baseConfig(), pages({PageId::TodayAgenda}), sources);

    const PageReadiness *agenda = findPageReadiness(health, PageId::TodayAgenda);
    TEST_ASSERT_NOT_NULL(agenda);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Ready),
                            static_cast<uint8_t>(agenda->state));
}

void test_finance_pages_report_required_setup_from_source_counts() {
    SourceConfigSummary sources;
    ConfigHealth health = buildConfigHealth(
        baseConfig(), pages({PageId::StockInfo, PageId::PortfolioSummary}), sources);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Required),
                            static_cast<uint8_t>(findPageReadiness(health, PageId::StockInfo)->state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Required),
                            static_cast<uint8_t>(findPageReadiness(health, PageId::PortfolioSummary)->state));

    sources.stockSymbols = 2;
    health = buildConfigHealth(
        baseConfig(), pages({PageId::StockInfo, PageId::PortfolioSummary}), sources);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Ready),
                            static_cast<uint8_t>(findPageReadiness(health, PageId::StockInfo)->state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Ready),
                            static_cast<uint8_t>(findPageReadiness(health, PageId::PortfolioSummary)->state));
}

void test_economic_calendar_requires_url_feed_not_region_codes() {
    AppConfig cfg = baseConfig();
    cfg.economicFeeds = "US, EU";
    SourceConfigSummary sources = sourceSummaryFromConfig(cfg);
    ConfigHealth health = buildConfigHealth(
        cfg, pages({PageId::EconomicCalendar}), sources);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Required),
                            static_cast<uint8_t>(findPageReadiness(health, PageId::EconomicCalendar)->state));

    cfg.economicFeeds = "https://example.com/economic.ics";
    sources = sourceSummaryFromConfig(cfg);
    health = buildConfigHealth(cfg, pages({PageId::EconomicCalendar}), sources);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Ready),
                            static_cast<uint8_t>(findPageReadiness(health, PageId::EconomicCalendar)->state));
}

void test_indoor_page_is_optional_when_sensor_source_is_disabled() {
    SourceConfigSummary sources;
    const ConfigHealth health = buildConfigHealth(
        baseConfig(), pages({PageId::IndoorClimate}), sources);

    const PageReadiness *indoor = findPageReadiness(health, PageId::IndoorClimate);
    TEST_ASSERT_NOT_NULL(indoor);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Optional),
                            static_cast<uint8_t>(indoor->state));
    TEST_ASSERT_TRUE(health.allEnabledPagesReady);
    TEST_ASSERT_EQUAL_UINT(1, health.optionalCount);
}

void test_focus_source_summary_carries_local_timer_configuration() {
    AppConfig cfg = baseConfig();
    cfg.focusLabel = "Deep Work";
    cfg.focusMinutes = 45;
    cfg.focusBreakMinutes = 10;
    cfg.focusSessionCount = 3;

    const SourceConfigSummary sources = sourceSummaryFromConfig(cfg);
    const ConfigHealth health = buildConfigHealth(
        cfg, pages({PageId::FocusClock}), sources);

    TEST_ASSERT_TRUE(sources.focusConfigured);
    TEST_ASSERT_EQUAL_UINT16(45, sources.focusMinutes);
    TEST_ASSERT_EQUAL_UINT16(10, sources.focusBreakMinutes);
    TEST_ASSERT_EQUAL_UINT8(3, sources.focusSessionCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageReadinessState::Ready),
                            static_cast<uint8_t>(findPageReadiness(health, PageId::FocusClock)->state));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_weather_page_is_ready_when_location_is_configured);
    RUN_TEST(test_calendar_page_requires_calendar_source_when_enabled);
    RUN_TEST(test_calendar_page_becomes_ready_with_enabled_calendar_source);
    RUN_TEST(test_finance_pages_report_required_setup_from_source_counts);
    RUN_TEST(test_economic_calendar_requires_url_feed_not_region_codes);
    RUN_TEST(test_indoor_page_is_optional_when_sensor_source_is_disabled);
    RUN_TEST(test_focus_source_summary_carries_local_timer_configuration);
    UNITY_END();
}

void loop() {}
