#include <unity.h>

#include "app/page/page_catalog.h"
#include "app/page/page_manager.h"
#include "app/page/page_catalog.cpp"
#include "app/page/page_manager.cpp"

namespace {
PageSettings twoPageSettings() {
    PageSettings settings = defaultPageSettings();
    settings.enabledMask = pageMask(PageId::Overview) | pageMask(PageId::TodayAgenda);
    settings.autoRotateMask = settings.enabledMask;
    settings.orderCount = 2;
    settings.order[0] = PageId::Overview;
    settings.order[1] = PageId::TodayAgenda;
    return settings;
}
}  // namespace

void test_manual_next_and_previous_wrap_enabled_pages() {
    PageManager manager(twoPageSettings());

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::TodayAgenda),
                            static_cast<uint8_t>(manager.nextManual()));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(manager.nextManual()));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::TodayAgenda),
                            static_cast<uint8_t>(manager.previousManual()));
}

void test_disabled_pages_are_skipped() {
    PageSettings settings = defaultPageSettings();
    settings.enabledMask = pageMask(PageId::Overview) | pageMask(PageId::MonthlyOverview);
    settings.autoRotateMask = settings.enabledMask;
    settings.orderCount = 4;
    settings.order[0] = PageId::Overview;
    settings.order[1] = PageId::TodayAgenda;
    settings.order[2] = PageId::WeeklyTimeline;
    settings.order[3] = PageId::MonthlyOverview;
    PageManager manager(settings);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::MonthlyOverview),
                            static_cast<uint8_t>(manager.nextManual()));
}

void test_page_manager_reports_enabled_page_position() {
    PageManager manager(twoPageSettings());

    TEST_ASSERT_EQUAL_UINT(2, manager.pageCount());
    TEST_ASSERT_EQUAL_UINT(1, manager.pageNumber(PageId::Overview));
    TEST_ASSERT_EQUAL_UINT(2, manager.pageNumber(PageId::TodayAgenda));
    TEST_ASSERT_EQUAL_UINT(1, manager.pageNumber(PageId::WorldClock));
}

void test_manual_navigation_does_not_move_auto_cursor() {
    PageManager manager(twoPageSettings());

    TEST_ASSERT_EQUAL_UINT(0, manager.autoCursor());
    manager.nextManual();
    TEST_ASSERT_EQUAL_UINT(0, manager.autoCursor());
}

void test_auto_rotation_queue_only_contains_selected_pages() {
    PageSettings settings = defaultPageSettings();
    settings.enabledMask = pageMask(PageId::Overview) | pageMask(PageId::TodayAgenda) | pageMask(PageId::WeeklyTimeline);
    settings.autoRotateMask = pageMask(PageId::Overview) | pageMask(PageId::WeeklyTimeline);
    settings.orderCount = 3;
    settings.order[0] = PageId::Overview;
    settings.order[1] = PageId::TodayAgenda;
    settings.order[2] = PageId::WeeklyTimeline;
    PageManager manager(settings);

    const auto queue = manager.rotationQueue();

    TEST_ASSERT_EQUAL_UINT(2, queue.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(queue[0]));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::WeeklyTimeline),
                            static_cast<uint8_t>(queue[1]));
}

void test_first_auto_rotation_advances_from_current_page() {
    PageManager manager(twoPageSettings());

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::TodayAgenda),
                            static_cast<uint8_t>(manager.nextAuto()));
    TEST_ASSERT_EQUAL_UINT(0, manager.autoCursor());
}

void test_rotation_interval_allowlist() {
    TEST_ASSERT_TRUE(isValidRotationIntervalMinutes(0));
    TEST_ASSERT_TRUE(isValidRotationIntervalMinutes(30));
    TEST_ASSERT_TRUE(isValidRotationIntervalMinutes(150));
    TEST_ASSERT_FALSE(isValidRotationIntervalMinutes(45));
}

void test_legacy_timezone_mapping() {
    TEST_ASSERT_EQUAL_STRING("Asia/Shanghai", timeZoneIdForUtcOffset(8).c_str());
    TEST_ASSERT_EQUAL_STRING("Etc/UTC", timeZoneIdForUtcOffset(0).c_str());
    TEST_ASSERT_EQUAL_STRING("America/New_York", timeZoneIdForUtcOffset(-5).c_str());
}

void test_invalid_rtc_state_restores_overview() {
    PageManager manager(twoPageSettings());
    const RtcPageState invalid{static_cast<PageId>(99), 3, 99, 0};

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(manager.restoreRtcState(invalid)));
}

void test_rtc_snapshot_preserves_page_cursor_and_version() {
    PageManager manager(twoPageSettings());
    manager.nextManual();
    const RtcPageState state = manager.snapshotRtcState(1234, 0xA5A5);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::TodayAgenda),
                            static_cast<uint8_t>(state.currentPage));
    TEST_ASSERT_EQUAL_UINT(0, state.autoCursor);
    TEST_ASSERT_EQUAL_UINT32(kDashboardConfigVersion, state.configVersion);
    TEST_ASSERT_EQUAL_INT64(1234, state.lastFullRefreshUtc);
    TEST_ASSERT_EQUAL_UINT32(0xA5A5, state.lastContentHash);
}

void test_button_page_actions_move_only_manual_navigation() {
    PageManager manager(twoPageSettings());

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::TodayAgenda),
                            static_cast<uint8_t>(applyButtonPageAction(manager, ButtonAction::NextPage)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(applyButtonPageAction(manager, ButtonAction::PreviousPage)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(applyButtonPageAction(manager, ButtonAction::SyncCurrent)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PageId::Overview),
                            static_cast<uint8_t>(applyButtonPageAction(manager, ButtonAction::None)));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_manual_next_and_previous_wrap_enabled_pages);
    RUN_TEST(test_disabled_pages_are_skipped);
    RUN_TEST(test_page_manager_reports_enabled_page_position);
    RUN_TEST(test_manual_navigation_does_not_move_auto_cursor);
    RUN_TEST(test_auto_rotation_queue_only_contains_selected_pages);
    RUN_TEST(test_first_auto_rotation_advances_from_current_page);
    RUN_TEST(test_rotation_interval_allowlist);
    RUN_TEST(test_legacy_timezone_mapping);
    RUN_TEST(test_invalid_rtc_state_restores_overview);
    RUN_TEST(test_rtc_snapshot_preserves_page_cursor_and_version);
    RUN_TEST(test_button_page_actions_move_only_manual_navigation);
    UNITY_END();
}

void loop() {}
