#include <unity.h>

void test_native_runner_is_available() {
    TEST_ASSERT_EQUAL_INT(400, 400);
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_native_runner_is_available);
    return UNITY_END();
}
