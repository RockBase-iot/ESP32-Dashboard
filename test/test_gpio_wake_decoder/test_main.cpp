#include <unity.h>

#include "app/power/gpio_wake_decoder.h"
#include "app/power/wake_coordinator.h"
#include "app/power/gpio_wake_decoder.cpp"

void test_gpio_wake_status_decodes_boot_even_if_button_released() {
    const uint64_t status = 1ULL << 0;

    const WakeSignal signal = decodeGpioWakeSignal(status, 0, 45, false, false);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WakeSignal::Boot),
                            static_cast<uint8_t>(signal));
}

void test_gpio_wake_status_decodes_user_even_if_button_released() {
    const uint64_t status = 1ULL << 45;

    const WakeSignal signal = decodeGpioWakeSignal(status, 0, 45, false, false);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WakeSignal::User),
                            static_cast<uint8_t>(signal));
}

void test_gpio_wake_falls_back_to_current_pin_level_without_status() {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WakeSignal::Boot),
                            static_cast<uint8_t>(decodeGpioWakeSignal(0, 0, 45, true, false)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WakeSignal::User),
                            static_cast<uint8_t>(decodeGpioWakeSignal(0, 0, 45, false, true)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(WakeSignal::None),
                            static_cast<uint8_t>(decodeGpioWakeSignal(0, 0, 45, false, false)));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_wake_status_decodes_boot_even_if_button_released);
    RUN_TEST(test_gpio_wake_status_decodes_user_even_if_button_released);
    RUN_TEST(test_gpio_wake_falls_back_to_current_pin_level_without_status);
    UNITY_END();
}

void loop() {}
