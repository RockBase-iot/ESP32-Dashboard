#include <unity.h>

#include "app/calendar/timezone_resolver.h"
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

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_new_york_dst_spring_forward_uses_daylight_offset);
    RUN_TEST(test_new_york_winter_uses_standard_offset);
    RUN_TEST(test_utc_values_are_not_shifted);
    UNITY_END();
}

void loop() {}
