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

void test_world_clock_formats_synced_local_date_for_header() {
    TEST_ASSERT_EQUAL_STRING("MON JUL 20, 2026",
                             formatWorldClockDateLabel(1784551320LL, "Asia/Shanghai").c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_timezone_catalog_maps_common_iana_names_to_posix);
    RUN_TEST(test_world_clock_builds_four_city_slots_with_day_delta);
    RUN_TEST(test_world_clock_formats_synced_local_date_for_header);
    UNITY_END();
}

void loop() {}
