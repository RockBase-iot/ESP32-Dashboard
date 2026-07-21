#include <unity.h>

#include "app/input/button_controller.h"
#include "app/input/button_controller.cpp"

void test_boot_short_press_goes_to_next_page_on_release() {
    ButtonController buttons;

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(buttons.update(ButtonId::Boot, true, 0)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(buttons.update(ButtonId::Boot, true, 50)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(buttons.update(ButtonId::Boot, false, 300)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::NextPage),
                            static_cast<uint8_t>(buttons.update(ButtonId::Boot, false, 350)));
}

void test_user_short_press_goes_to_previous_page_on_release() {
    ButtonController buttons;

    buttons.update(ButtonId::User, true, 0);
    buttons.update(ButtonId::User, true, 50);
    buttons.update(ButtonId::User, false, 300);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::PreviousPage),
                            static_cast<uint8_t>(buttons.update(ButtonId::User, false, 350)));
}

void test_user_two_second_press_syncs_current_page() {
    ButtonController buttons;

    buttons.update(ButtonId::User, true, 0);
    buttons.update(ButtonId::User, true, 50);
    buttons.update(ButtonId::User, false, 2150);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::SyncCurrent),
                            static_cast<uint8_t>(buttons.update(ButtonId::User, false, 2200)));
}

void test_boot_two_second_press_opens_config_window() {
    ButtonController buttons;

    buttons.update(ButtonId::Boot, true, 0);
    buttons.update(ButtonId::Boot, true, 50);
    buttons.update(ButtonId::Boot, false, 2150);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::OpenConfig),
                            static_cast<uint8_t>(buttons.update(ButtonId::Boot, false, 2200)));
}

void test_boot_six_second_press_uses_recovery_ap_instead_of_config() {
    ButtonController buttons;

    buttons.update(ButtonId::Boot, true, 0);
    buttons.update(ButtonId::Boot, true, 50);
    buttons.update(ButtonId::Boot, false, 6150);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::RecoveryAp),
                            static_cast<uint8_t>(buttons.update(ButtonId::Boot, false, 6200)));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_boot_short_press_goes_to_next_page_on_release);
    RUN_TEST(test_user_short_press_goes_to_previous_page_on_release);
    RUN_TEST(test_user_two_second_press_syncs_current_page);
    RUN_TEST(test_boot_two_second_press_opens_config_window);
    RUN_TEST(test_boot_six_second_press_uses_recovery_ap_instead_of_config);
    UNITY_END();
}

void loop() {}
