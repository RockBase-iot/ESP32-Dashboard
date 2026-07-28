#include <unity.h>

#include "app/calendar/timezone_resolver.h"
#include "app/time/timezone_catalog.cpp"
#include "app/calendar/timezone_resolver.cpp"

void test_new_york_dst_spring_forward_uses_daylight_offset() {
    TEST_ASSERT_EQUAL_INT64(1784552400LL,
                            TimezoneResolver().toUtc("America/New_York", LocalDateTime{2026, 7, 20, 9, 0, 0}));
}

void test_new_york_winter_uses_standard_offset() {
    TEST_ASSERT_EQUAL_INT64(1768917600LL,
                            TimezoneResolver().toUtc("America/New_York", LocalDateTime{2026, 1, 20, 9, 0, 0}));
}

void test_utc_values_are_not_shifted() {
    TEST_ASSERT_EQUAL_INT64(1784538000LL,
                            TimezoneResolver().toUtc("Etc/UTC", LocalDateTime{2026, 7, 20, 9, 0, 0}));
}

void test_windows_timezone_names_resolve_to_iana_rules() {
    TEST_ASSERT_EQUAL_INT64(1784563200LL,
                            TimezoneResolver().toUtc("Pacific Standard Time", LocalDateTime{2026, 7, 20, 9, 0, 0}));
    TEST_ASSERT_EQUAL(-7 * 3600,
                      TimezoneResolver().offsetSeconds("Pacific Standard Time", 1784563200LL));
}

void test_unknown_timezone_falls_back_to_utc_without_shift() {
    TEST_ASSERT_EQUAL_INT64(1784538000LL,
                            TimezoneResolver().toUtc("Mars/Base", LocalDateTime{2026, 7, 20, 9, 0, 0}));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_new_york_dst_spring_forward_uses_daylight_offset);
    RUN_TEST(test_new_york_winter_uses_standard_offset);
    RUN_TEST(test_utc_values_are_not_shifted);
    RUN_TEST(test_windows_timezone_names_resolve_to_iana_rules);
    RUN_TEST(test_unknown_timezone_falls_back_to_utc_without_shift);
    UNITY_END();
}

void loop() {}
