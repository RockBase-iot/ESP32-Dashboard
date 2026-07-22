#include <unity.h>

#include "utils/light_wake.h"
#include "utils/light_wake.cpp"

// Timer fired, no GPIO activity at all → the inactivity budget expired.
void test_timer_only_returns_timeout() {
    const LightWake wake = classifyLightWake(/*timerFired=*/true, /*gpioFired=*/false,
                                             /*gpioWakeStatus=*/0,
                                             /*bootPin=*/0, /*userPin=*/45,
                                             /*bootPressedNow=*/false, /*userPressedNow=*/false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::Timeout),
                            static_cast<uint8_t>(wake));
}

// GPIO status bit set for BOOT even though the button was already released
// (fast press/release) → still classified as a BOOT wake.
void test_gpio_status_decodes_boot_even_if_button_released() {
    const LightWake wake = classifyLightWake(false, true, 1ULL << 0,
                                             0, 45, false, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::BootButton),
                            static_cast<uint8_t>(wake));
}

// GPIO45 lives in the high status word; the caller passes a merged 64-bit map.
void test_gpio_status_decodes_user_even_if_button_released() {
    const LightWake wake = classifyLightWake(false, true, 1ULL << 45,
                                             0, 45, false, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::UserButton),
                            static_cast<uint8_t>(wake));
}

// No status bits (hardware register read zero) → fall back to live pin level.
void test_pin_level_fallback_when_status_empty() {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::BootButton),
                            static_cast<uint8_t>(classifyLightWake(false, true, 0,
                                                                   0, 45, true, false)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::UserButton),
                            static_cast<uint8_t>(classifyLightWake(false, true, 0,
                                                                   0, 45, false, true)));
}

// A press landing exactly at the inactivity deadline is user intent:
// buttons win over the timeout, the caller resets its budget anyway.
void test_button_beats_timer_at_deadline() {
    const LightWake wake = classifyLightWake(true, true, 1ULL << 0,
                                             0, 45, false, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::BootButton),
                            static_cast<uint8_t>(wake));
}

// GPIO fired for a pin that is neither button → unattributed wake.
void test_unattributed_gpio_wake_returns_other() {
    const LightWake wake = classifyLightWake(false, true, 1ULL << 7,
                                             0, 45, false, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::Other),
                            static_cast<uint8_t>(wake));
}

// No timer, no GPIO, nothing pressed → spurious wake.
void test_spurious_wake_returns_other() {
    const LightWake wake = classifyLightWake(false, false, 0,
                                             0, 45, false, false);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::Other),
                            static_cast<uint8_t>(wake));
}

// Absent pins (0xFF) must never attribute a wake to them.
void test_absent_pins_never_match() {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::Other),
                            static_cast<uint8_t>(classifyLightWake(false, true, 1ULL << 0,
                                                                   0xFF, 0xFF, true, true)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(LightWake::Timeout),
                            static_cast<uint8_t>(classifyLightWake(true, false, 0,
                                                                   0xFF, 0xFF, false, false)));
}

// ── Interactive window exit policy ───────────────────────────────────────────

// A Timeout wake always ends the window.
void test_window_exits_on_timeout() {
    TEST_ASSERT_TRUE(interactiveWindowShouldExit(LightWake::Timeout, 20000, 30000));
    TEST_ASSERT_TRUE(interactiveWindowShouldExit(LightWake::Timeout, 30000, 30000));
}

// A button wake inside the budget keeps the window alive.
void test_window_survives_button_wake_inside_budget() {
    TEST_ASSERT_FALSE(interactiveWindowShouldExit(LightWake::BootButton, 5000, 30000));
    TEST_ASSERT_FALSE(interactiveWindowShouldExit(LightWake::UserButton, 29999, 30000));
}

// A spurious wake inside the budget just re-arms the sleep.
void test_window_survives_spurious_wake_inside_budget() {
    TEST_ASSERT_FALSE(interactiveWindowShouldExit(LightWake::Other, 10000, 30000));
}

// Budget already consumed (e.g. a long render after the last activity) ends
// the window even when a button woke us.
void test_window_exits_when_budget_already_exceeded() {
    TEST_ASSERT_TRUE(interactiveWindowShouldExit(LightWake::BootButton, 30000, 30000));
    TEST_ASSERT_TRUE(interactiveWindowShouldExit(LightWake::UserButton, 45000, 30000));
    TEST_ASSERT_TRUE(interactiveWindowShouldExit(LightWake::Other, 31000, 30000));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_timer_only_returns_timeout);
    RUN_TEST(test_gpio_status_decodes_boot_even_if_button_released);
    RUN_TEST(test_gpio_status_decodes_user_even_if_button_released);
    RUN_TEST(test_pin_level_fallback_when_status_empty);
    RUN_TEST(test_button_beats_timer_at_deadline);
    RUN_TEST(test_unattributed_gpio_wake_returns_other);
    RUN_TEST(test_spurious_wake_returns_other);
    RUN_TEST(test_absent_pins_never_match);
    RUN_TEST(test_window_exits_on_timeout);
    RUN_TEST(test_window_survives_button_wake_inside_budget);
    RUN_TEST(test_window_survives_spurious_wake_inside_budget);
    RUN_TEST(test_window_exits_when_budget_already_exceeded);
    UNITY_END();
}

void loop() {}
