#include <unity.h>

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

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_unreferenced_provider_is_not_selected);
    RUN_TEST(test_retry_after_blocks_rate_limited_provider);
    RUN_TEST(test_ttl_controls_due_provider_selection);
    return UNITY_END();
}
