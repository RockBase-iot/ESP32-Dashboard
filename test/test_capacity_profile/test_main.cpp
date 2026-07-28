#include <unity.h>

#include "app/memory/capacity_profile.h"
#include "app/memory/capacity_profile.cpp"

void test_safe_profile_without_psram() {
    const auto profile = detectCapacity(false, 0);

    TEST_ASSERT_EQUAL_UINT8(10, profile.calendarSlots);
    TEST_ASSERT_EQUAL_UINT8(6, profile.defaultEnabledSources);
    TEST_ASSERT_EQUAL_UINT16(250, profile.maxExpandedEvents);
    TEST_ASSERT_EQUAL_UINT32(512 * 1024, profile.maxSourceBytes);
    TEST_ASSERT_EQUAL_UINT32(80 * 1024, profile.minInternalFreeHeap);
    TEST_ASSERT_FALSE(profile.extended);
}

void test_safe_profile_when_psram_is_too_small() {
    const auto profile = detectCapacity(true, 6 * 1024 * 1024);

    TEST_ASSERT_EQUAL_UINT8(10, profile.calendarSlots);
    TEST_ASSERT_EQUAL_UINT16(250, profile.maxExpandedEvents);
    TEST_ASSERT_EQUAL_UINT32(512 * 1024, profile.maxSourceBytes);
    TEST_ASSERT_FALSE(profile.extended);
}

void test_extended_profile_with_8mb_psram() {
    const auto profile = detectCapacity(true, 8 * 1024 * 1024);

    TEST_ASSERT_EQUAL_UINT8(20, profile.calendarSlots);
    TEST_ASSERT_EQUAL_UINT8(6, profile.defaultEnabledSources);
    TEST_ASSERT_EQUAL_UINT16(1000, profile.maxExpandedEvents);
    TEST_ASSERT_EQUAL_UINT32(2 * 1024 * 1024, profile.maxSourceBytes);
    TEST_ASSERT_EQUAL_UINT32(80 * 1024, profile.minInternalFreeHeap);
    TEST_ASSERT_TRUE(profile.extended);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_safe_profile_without_psram);
    RUN_TEST(test_safe_profile_when_psram_is_too_small);
    RUN_TEST(test_extended_profile_with_8mb_psram);
    return UNITY_END();
}
