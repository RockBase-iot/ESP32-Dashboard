#include <unity.h>

#include "app/input/pending_button_action.h"
#include "app/input/pending_button_action.cpp"

void test_invalid_mailbox_consumes_as_none() {
    PendingButtonActionState state{0x12345678UL, static_cast<uint8_t>(ButtonAction::NextPage), 0, 1};

    const ButtonAction action = consumePendingButtonAction(state);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(action));
    TEST_ASSERT_EQUAL_UINT32(0, state.magic);
}

void test_stored_action_is_consumed_once() {
    PendingButtonActionState state{};

    storePendingButtonAction(state, ButtonAction::PreviousPage);

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::PreviousPage),
                            static_cast<uint8_t>(consumePendingButtonAction(state)));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(consumePendingButtonAction(state)));
}

void test_corrupt_action_copy_is_rejected() {
    PendingButtonActionState state{};
    storePendingButtonAction(state, ButtonAction::NextPage);
    state.inverseAction = 0;

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ButtonAction::None),
                            static_cast<uint8_t>(consumePendingButtonAction(state)));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_invalid_mailbox_consumes_as_none);
    RUN_TEST(test_stored_action_is_consumed_once);
    RUN_TEST(test_corrupt_action_copy_is_rejected);
    UNITY_END();
}

void loop() {}

