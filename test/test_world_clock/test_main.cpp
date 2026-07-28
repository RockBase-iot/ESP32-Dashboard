#include <unity.h>

#include "app/time/timezone_catalog.h"
#include "app/time/timezone_catalog.cpp"
#include "app/time/world_clock_model.h"
#include "app/time/world_clock_model.cpp"

void test_timezone_catalog_maps_common_iana_names_to_posix() {
    TEST_ASSERT_EQUAL_STRING("EST5EDT,M3.2.0/2,M11.1.0/2",
                             timezonePosixRule("America/New_York").c_str());
    TEST_ASSERT_EQUAL_STRING("CST-8", timezonePosixRule("Asia/Shanghai").c_str());
}

void test_world_clock_builds_four_city_slots_with_day_delta() {
    WorldClockConfig cfg;
    cfg.twentyFourHour = true;
    cfg.zones.push_back({"New York", "America/New_York"});
    cfg.zones.push_back({"London", "Europe/London"});
    cfg.zones.push_back({"Berlin", "Europe/Berlin"});
    cfg.zones.push_back({"Shanghai", "Asia/Shanghai"});

    const auto model = buildWorldClock(cfg, 1784538000LL);

    TEST_ASSERT_EQUAL_UINT32(4, model.clocks.size());
    TEST_ASSERT_EQUAL_STRING("New York", model.clocks[0].label.c_str());
    TEST_ASSERT_EQUAL_STRING("05:00", model.clocks[0].timeText.c_str());
    TEST_ASSERT_EQUAL_STRING("17:00", model.clocks[3].timeText.c_str());
}

void test_world_clock_config_parses_user_zone_list_in_display_order() {
    const auto cfg = worldClockConfigFromZonesText(
        "Europe/Paris|Paris\nAsia/Tokyo|Tokyo,America/Los_Angeles|Los Angeles,Australia/Sydney|Sydney,Europe/London|London",
        "Deep Work",
        true);

    TEST_ASSERT_TRUE(cfg.twentyFourHour);
    TEST_ASSERT_EQUAL_UINT32(4, cfg.zones.size());
    TEST_ASSERT_EQUAL_STRING("Paris", cfg.zones[0].label.c_str());
    TEST_ASSERT_EQUAL_STRING("Europe/Paris", cfg.zones[0].timezoneId.c_str());
    TEST_ASSERT_EQUAL_STRING("Los Angeles", cfg.zones[2].label.c_str());
    TEST_ASSERT_EQUAL_STRING("America/Los_Angeles", cfg.zones[2].timezoneId.c_str());
    TEST_ASSERT_EQUAL_STRING("Deep Work", cfg.focusLabel.c_str());
}

void test_world_clock_config_accepts_label_first_timezone_rows() {
    const auto cfg = worldClockConfigFromZonesText(
        "Shanghai | Asia/Shanghai\nNew York | America/New_York\nLondon | Europe/London\nTokyo | Asia/Tokyo",
        "",
        true);

    TEST_ASSERT_EQUAL_UINT32(4, cfg.zones.size());
    TEST_ASSERT_EQUAL_STRING("Shanghai", cfg.zones[0].label.c_str());
    TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", cfg.zones[0].timezoneId.c_str());
    TEST_ASSERT_EQUAL_STRING("New York", cfg.zones[1].label.c_str());
    TEST_ASSERT_EQUAL_STRING("America/New_York", cfg.zones[1].timezoneId.c_str());
}

void test_world_clock_config_skips_blank_rows_from_four_row_form() {
    const auto cfg = worldClockConfigFromZonesText(
        "Shanghai | Asia/Shanghai\n\nLondon | Europe/London\n",
        "",
        true);

    TEST_ASSERT_EQUAL_UINT32(2, cfg.zones.size());
    TEST_ASSERT_EQUAL_STRING("Shanghai", cfg.zones[0].label.c_str());
    TEST_ASSERT_EQUAL_STRING("London", cfg.zones[1].label.c_str());
}

void test_timezone_catalog_supports_web_common_city_choices() {
    TEST_ASSERT_EQUAL(2 * 3600, timezoneOffsetSecondsAtUtc("Europe/Paris", 1784538000LL));
    TEST_ASSERT_EQUAL(10 * 3600, timezoneOffsetSecondsAtUtc("Australia/Sydney", 1784538000LL));
    TEST_ASSERT_EQUAL(9 * 3600, timezoneOffsetSecondsAtUtc("Asia/Seoul", 1784538000LL));
}

void test_world_clock_formats_chrome_time_with_local_date() {
    TEST_ASSERT_EQUAL_STRING("MON 09:42 JUL 20, 2026",
                             formatWorldClockChromeTimeLabel(1784551320LL,
                                                             "Asia/Shanghai").c_str());
}

void test_world_clock_formats_synced_local_date_for_header() {
    TEST_ASSERT_EQUAL_STRING("MON JUL 20, 2026",
                             formatWorldClockDateLabel(1784551320LL, "Asia/Shanghai").c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_timezone_catalog_maps_common_iana_names_to_posix);
    RUN_TEST(test_world_clock_builds_four_city_slots_with_day_delta);
    RUN_TEST(test_world_clock_config_parses_user_zone_list_in_display_order);
    RUN_TEST(test_world_clock_config_accepts_label_first_timezone_rows);
    RUN_TEST(test_world_clock_config_skips_blank_rows_from_four_row_form);
    RUN_TEST(test_timezone_catalog_supports_web_common_city_choices);
    RUN_TEST(test_world_clock_formats_chrome_time_with_local_date);
    RUN_TEST(test_world_clock_formats_synced_local_date_for_header);
    UNITY_END();
}

void loop() {}
