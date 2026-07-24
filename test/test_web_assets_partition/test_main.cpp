#include <unity.h>

#include "app/web/web_server.h"

void test_web_assets_mount_uses_uploaded_littlefs_partition_label() {
    TEST_ASSERT_EQUAL_STRING("littlefs", kWebAssetsPartitionLabel);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_web_assets_mount_uses_uploaded_littlefs_partition_label);
    UNITY_END();
}

void loop() {}
