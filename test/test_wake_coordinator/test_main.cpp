#include <unity.h>

#include "app/power/wake_coordinator.h"
#include "app/power/wake_coordinator.cpp"

static_assert(static_cast<uint8_t>(PowerState::DeepSleep) == 1,
              "Intermediate sleep state must be removed from the device power states");

void test_29_seconds_inactivity_stays_interactive() {
    WakeCoordinator coordinator;
    WakeInputs input;
    input.currentState = PowerState::Interactive;
    input.signal = WakeSignal::Inactivity;
    input.secondsSinceActivity = 29;
    input.deepSleepTimerUs = 300ULL * 1000000ULL;

    const auto decision = coordinator.decide(input);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::Interactive),
                            static_cast<uint8_t>(decision.nextState));
}

void test_30_seconds_inactivity_enters_deep_sleep() {
    WakeCoordinator coordinator;
    WakeInputs input;
    input.currentState = PowerState::Interactive;
    input.signal = WakeSignal::Inactivity;
    input.secondsSinceActivity = 30;
    input.deepSleepTimerUs = 300ULL * 1000000ULL;

    const auto decision = coordinator.decide(input);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::DeepSleep),
                            static_cast<uint8_t>(decision.nextState));
    TEST_ASSERT_EQUAL_UINT64(300ULL * 1000000ULL, decision.timerWakeUs);
}

void test_night_window_goes_directly_to_deep_sleep() {
    WakeCoordinator coordinator;
    WakeInputs input;
    input.currentState = PowerState::Interactive;
    input.signal = WakeSignal::NightWindow;
    input.nightWindow = true;
    input.deepSleepTimerUs = 1800ULL * 1000000ULL;

    const auto decision = coordinator.decide(input);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::DeepSleep),
                            static_cast<uint8_t>(decision.nextState));
}

void test_deep_sleep_boot_and_timer_wake_to_interactive() {
    WakeCoordinator coordinator;
    WakeInputs input;
    input.currentState = PowerState::DeepSleep;

    input.signal = WakeSignal::Boot;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::Interactive),
                            static_cast<uint8_t>(coordinator.decide(input).nextState));

    input.signal = WakeSignal::Timer;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::Interactive),
                            static_cast<uint8_t>(coordinator.decide(input).nextState));
}

void test_deep_sleep_user_signal_is_not_assumed_as_wake_source() {
    WakeCoordinator coordinator;
    WakeInputs input;
    input.currentState = PowerState::DeepSleep;
    input.signal = WakeSignal::User;

    const auto decision = coordinator.decide(input);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PowerState::DeepSleep),
                            static_cast<uint8_t>(decision.nextState));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_29_seconds_inactivity_stays_interactive);
    RUN_TEST(test_30_seconds_inactivity_enters_deep_sleep);
    RUN_TEST(test_night_window_goes_directly_to_deep_sleep);
    RUN_TEST(test_deep_sleep_boot_and_timer_wake_to_interactive);
    RUN_TEST(test_deep_sleep_user_signal_is_not_assumed_as_wake_source);
    UNITY_END();
}

void loop() {}
