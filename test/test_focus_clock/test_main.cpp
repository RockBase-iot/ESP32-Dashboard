#include <unity.h>

#include "app/time/focus_clock_model.h"
#include "app/time/focus_clock_model.cpp"
#include "app/time/focus_clock_controller.h"
#include "app/time/focus_clock_controller.cpp"
#include "app/input/button_controller.h"
#include "app/page/page_catalog.h"
#include "app/time/timezone_catalog.h"
#include "app/time/timezone_catalog.cpp"
#include "utils/light_wake.h"

void test_ready_focus_clock_snapshot_uses_configured_25_5_defaults() {
    FocusClockConfig cfg;
    cfg.label = "Deep Work";
    cfg.focusMinutes = 25;
    cfg.breakMinutes = 5;
    cfg.sessionCount = 4;

    FocusClockRuntimeState state;
    const auto snapshot = focusClockPageSnapshotAt(1784551320LL, "Asia/Shanghai",
                                                   10, 16, cfg, state);

    TEST_ASSERT_EQUAL_STRING("FOCUS CLOCK", snapshot.title.c_str());
    TEST_ASSERT_EQUAL_STRING("25 MIN", snapshot.countdownText.c_str());
    TEST_ASSERT_EQUAL_STRING("READY", snapshot.statusText.c_str());
    TEST_ASSERT_EQUAL_STRING("CYCLE 1 OF 4", snapshot.cycleText.c_str());
    TEST_ASSERT_EQUAL_STRING("25 MIN FOCUS", snapshot.focusDurationText.c_str());
    TEST_ASSERT_EQUAL_STRING("5 MIN BREAK", snapshot.breakDurationText.c_str());
    TEST_ASSERT_EQUAL_STRING("--:--", snapshot.nextBreakText.c_str());
    TEST_ASSERT_EQUAL_STRING("--:--", snapshot.endTimeText.c_str());
    TEST_ASSERT_EQUAL_STRING("USER HOLD 2S START / STOP", snapshot.controlText.c_str());
    TEST_ASSERT_EQUAL_STRING("10/16", snapshot.pageIndicator.c_str());
}

void test_active_focus_clock_snapshot_counts_down_and_formats_schedule() {
    FocusClockConfig cfg;
    cfg.focusMinutes = 25;
    cfg.breakMinutes = 5;
    cfg.sessionCount = 4;

    const int64_t startUtc = 1784551320LL;
    const FocusClockRuntimeState state = startFocusClockSession(cfg, startUtc);
    const auto snapshot = focusClockPageSnapshotAt(startUtc + 5 * 60, "Asia/Shanghai",
                                                   10, 16, cfg, state);

    TEST_ASSERT_EQUAL_STRING("20 MIN", snapshot.countdownText.c_str());
    TEST_ASSERT_EQUAL_STRING("IN FOCUS", snapshot.statusText.c_str());
    TEST_ASSERT_EQUAL_STRING("CYCLE 1 OF 4", snapshot.cycleText.c_str());
    TEST_ASSERT_EQUAL_STRING("10:07", snapshot.nextBreakText.c_str());
    TEST_ASSERT_EQUAL_STRING("10:12", snapshot.endTimeText.c_str());
    TEST_ASSERT_TRUE(snapshot.active);
    TEST_ASSERT_FALSE(snapshot.inBreak);
}

void test_focus_clock_snapshot_switches_to_break_phase_after_focus_block() {
    FocusClockConfig cfg;
    cfg.focusMinutes = 25;
    cfg.breakMinutes = 5;
    cfg.sessionCount = 4;

    const int64_t startUtc = 1784551320LL;
    const FocusClockRuntimeState state = startFocusClockSession(cfg, startUtc);
    const auto snapshot = focusClockPageSnapshotAt(startUtc + 26 * 60, "Asia/Shanghai",
                                                   10, 16, cfg, state);

    TEST_ASSERT_EQUAL_STRING("4 MIN", snapshot.countdownText.c_str());
    TEST_ASSERT_EQUAL_STRING("BREAK", snapshot.statusText.c_str());
    TEST_ASSERT_EQUAL_STRING("CYCLE 1 OF 4", snapshot.cycleText.c_str());
    TEST_ASSERT_TRUE(snapshot.active);
    TEST_ASSERT_TRUE(snapshot.inBreak);
}

void test_focus_clock_stop_clears_active_state() {
    FocusClockConfig cfg;
    FocusClockRuntimeState state = startFocusClockSession(cfg, 1784551320LL);

    state = stopFocusClockSession();

    const auto snapshot = focusClockPageSnapshotAt(1784551320LL + 60, "Asia/Shanghai",
                                                   10, 16, cfg, state);
    TEST_ASSERT_FALSE(snapshot.active);
    TEST_ASSERT_EQUAL_STRING("READY", snapshot.statusText.c_str());
    TEST_ASSERT_EQUAL_STRING("25 MIN", snapshot.countdownText.c_str());
}

void test_focus_clock_controller_maps_user_long_press_to_page_local_toggle() {
    FocusClockConfig cfg;
    FocusClockRuntimeState state;

    TEST_ASSERT_FALSE(applyFocusClockButtonAction(PageId::WorldClock, ButtonAction::SyncCurrent,
                                                  cfg, state, 1784551320LL));
    TEST_ASSERT_FALSE(state.active);

    TEST_ASSERT_TRUE(applyFocusClockButtonAction(PageId::FocusClock, ButtonAction::SyncCurrent,
                                                 cfg, state, 1784551320LL));
    TEST_ASSERT_TRUE(state.active);
    TEST_ASSERT_EQUAL_INT64(1784551320LL, state.startedUtc);

    TEST_ASSERT_TRUE(applyFocusClockButtonAction(PageId::FocusClock, ButtonAction::SyncCurrent,
                                                 cfg, state, 1784551380LL));
    TEST_ASSERT_FALSE(state.active);
}

void test_active_focus_clock_blocks_page_navigation_until_complete_or_stopped() {
    FocusClockConfig cfg;
    FocusClockRuntimeState state = startFocusClockSession(cfg, 1784551320LL);

    TEST_ASSERT_TRUE(focusClockBlocksButtonAction(PageId::FocusClock,
                                                  ButtonAction::NextPage,
                                                  cfg, state,
                                                  1784551380LL));
    TEST_ASSERT_TRUE(focusClockBlocksButtonAction(PageId::FocusClock,
                                                  ButtonAction::PreviousPage,
                                                  cfg, state,
                                                  1784551380LL));
    TEST_ASSERT_FALSE(focusClockBlocksButtonAction(PageId::FocusClock,
                                                   ButtonAction::SyncCurrent,
                                                   cfg, state,
                                                   1784551380LL));
    TEST_ASSERT_FALSE(focusClockBlocksButtonAction(PageId::WorldClock,
                                                   ButtonAction::NextPage,
                                                   cfg, state,
                                                   1784551380LL));

    state = stopFocusClockSession();
    TEST_ASSERT_FALSE(focusClockBlocksButtonAction(PageId::FocusClock,
                                                   ButtonAction::NextPage,
                                                   cfg, state,
                                                   1784551380LL));
}

void test_active_focus_clock_uses_minute_light_sleep_refresh_ticks() {
    FocusClockConfig cfg;
    cfg.focusMinutes = 1;
    cfg.breakMinutes = 1;
    cfg.sessionCount = 1;
    const FocusClockRuntimeState state = startFocusClockSession(cfg, 1784551320LL);

    TEST_ASSERT_EQUAL_UINT32(60000UL,
                             focusClockNextRefreshMs(cfg, state, 1784551320LL));
    TEST_ASSERT_EQUAL_UINT32(30000UL,
                             focusClockNextRefreshMs(cfg, state, 1784551410LL));
    TEST_ASSERT_EQUAL_UINT32(0UL,
                             focusClockNextRefreshMs(cfg, state, 1784551440LL));
}

void test_focus_clock_refresh_deadline_stays_fixed_across_button_poll_ticks() {
    FocusClockConfig cfg;
    cfg.focusMinutes = 25;
    cfg.breakMinutes = 5;
    cfg.sessionCount = 1;
    const FocusClockRuntimeState state = startFocusClockSession(cfg, 1784551320LL);

    const int64_t firstDeadline = focusClockNextRefreshUtc(cfg, state, 1784551320LL);

    TEST_ASSERT_EQUAL_INT64(1784551380LL, firstDeadline);
    TEST_ASSERT_TRUE(1784551321LL < firstDeadline);
    TEST_ASSERT_EQUAL_INT64(1784551440LL,
                            focusClockNextRefreshUtc(cfg, state, firstDeadline));
}

void test_focus_clock_light_wake_stop_is_user_only_and_timeboxed() {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(focusClockActionFromLightWake(
                                LightWake::UserButton, 1999UL)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::SyncCurrent),
                            static_cast<uint8_t>(focusClockActionFromLightWake(
                                LightWake::UserButton, 2000UL)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(focusClockActionFromLightWake(
                                LightWake::BootButton, 6500UL)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(focusClockActionFromLightWake(
                                LightWake::Timeout, 6500UL)));
}

void test_focus_clock_light_wake_is_only_a_focus_local_stop_action() {
    FocusClockConfig cfg;
    FocusClockRuntimeState state = startFocusClockSession(cfg, 1784551320LL);

    TEST_ASSERT_FALSE(applyFocusClockButtonAction(
        PageId::FocusClock,
        focusClockActionFromLightWake(LightWake::BootButton, 3000UL),
        cfg, state, 1784551330LL));
    TEST_ASSERT_TRUE(state.active);

    TEST_ASSERT_FALSE(applyFocusClockButtonAction(
        PageId::FocusClock,
        focusClockActionFromLightWake(LightWake::UserButton, 1999UL),
        cfg, state, 1784551330LL));
    TEST_ASSERT_TRUE(state.active);

    TEST_ASSERT_TRUE(applyFocusClockButtonAction(
        PageId::FocusClock,
        focusClockActionFromLightWake(LightWake::UserButton, 2000UL),
        cfg, state, 1784551330LL));
    TEST_ASSERT_FALSE(state.active);
}

void test_active_focus_clock_suppresses_light_wake_from_global_navigation() {
    FocusClockConfig cfg;
    FocusClockRuntimeState state = startFocusClockSession(cfg, 1784551320LL);

    TEST_ASSERT_TRUE(focusClockSuppressesLightWake(PageId::FocusClock,
                                                  LightWake::BootButton,
                                                  cfg, state,
                                                  1784551330LL));
    TEST_ASSERT_TRUE(focusClockSuppressesLightWake(PageId::FocusClock,
                                                  LightWake::UserButton,
                                                  cfg, state,
                                                  1784551330LL));
    TEST_ASSERT_FALSE(focusClockSuppressesLightWake(PageId::WorldClock,
                                                   LightWake::UserButton,
                                                   cfg, state,
                                                   1784551330LL));

    state = stopFocusClockSession();
    TEST_ASSERT_FALSE(focusClockSuppressesLightWake(PageId::FocusClock,
                                                   LightWake::UserButton,
                                                   cfg, state,
                                                   1784551330LL));
}

void test_focus_clock_awake_hold_action_fires_without_waiting_for_release() {
    FocusClockConfig cfg;
    FocusClockRuntimeState state;

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(focusClockActionFromAwakeHold(
                                PageId::FocusClock, true, 1999UL, false)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::SyncCurrent),
                            static_cast<uint8_t>(focusClockActionFromAwakeHold(
                                PageId::FocusClock, true, 2000UL, false)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(focusClockActionFromAwakeHold(
                                PageId::FocusClock, true, 2500UL, true)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(focusClockActionFromAwakeHold(
                                PageId::WorldClock, true, 2500UL, false)));

    TEST_ASSERT_TRUE(applyFocusClockButtonAction(
        PageId::FocusClock,
        focusClockActionFromAwakeHold(PageId::FocusClock, true, 2000UL, false),
        cfg, state, 1784551320LL));
    TEST_ASSERT_TRUE(state.active);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_ready_focus_clock_snapshot_uses_configured_25_5_defaults);
    RUN_TEST(test_active_focus_clock_snapshot_counts_down_and_formats_schedule);
    RUN_TEST(test_focus_clock_snapshot_switches_to_break_phase_after_focus_block);
    RUN_TEST(test_focus_clock_stop_clears_active_state);
    RUN_TEST(test_focus_clock_controller_maps_user_long_press_to_page_local_toggle);
    RUN_TEST(test_active_focus_clock_blocks_page_navigation_until_complete_or_stopped);
    RUN_TEST(test_active_focus_clock_uses_minute_light_sleep_refresh_ticks);
    RUN_TEST(test_focus_clock_refresh_deadline_stays_fixed_across_button_poll_ticks);
    RUN_TEST(test_focus_clock_light_wake_stop_is_user_only_and_timeboxed);
    RUN_TEST(test_focus_clock_light_wake_is_only_a_focus_local_stop_action);
    RUN_TEST(test_active_focus_clock_suppresses_light_wake_from_global_navigation);
    RUN_TEST(test_focus_clock_awake_hold_action_fires_without_waiting_for_release);
    UNITY_END();
}

void loop() {}
