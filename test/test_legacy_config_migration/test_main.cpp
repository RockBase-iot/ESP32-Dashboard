#include <unity.h>

#include <string>

#include "app/config/legacy_config_migration.h"
#include "app/config/legacy_config_migration.cpp"

namespace {
class MemoryLegacyConfigStore final : public ILegacyConfigStore {
public:
    LegacyKeyEraseResult eraseKey(const char *key) override {
        ++eraseCalls;
        erasedKey = key ? key : "";
        return eraseResult;
    }

    bool commit() override {
        ++commitCalls;
        return commitResult;
    }

    LegacyKeyEraseResult eraseResult = LegacyKeyEraseResult::NotFound;
    bool commitResult = true;
    int eraseCalls = 0;
    int commitCalls = 0;
    std::string erasedKey;
};
}  // namespace

void test_existing_admin_token_is_removed_and_committed() {
    MemoryLegacyConfigStore store;
    store.eraseResult = LegacyKeyEraseResult::Removed;

    const LegacyConfigMigrationResult result = migrateLegacyAccessCredential(store);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(LegacyConfigMigrationResult::Removed),
                          static_cast<int>(result));
    TEST_ASSERT_EQUAL_STRING("AdminTokH", store.erasedKey.c_str());
    TEST_ASSERT_EQUAL_INT(1, store.eraseCalls);
    TEST_ASSERT_EQUAL_INT(1, store.commitCalls);
}

void test_missing_admin_token_does_not_commit() {
    MemoryLegacyConfigStore store;
    store.eraseResult = LegacyKeyEraseResult::NotFound;

    const LegacyConfigMigrationResult result = migrateLegacyAccessCredential(store);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(LegacyConfigMigrationResult::NotPresent),
                          static_cast<int>(result));
    TEST_ASSERT_EQUAL_INT(1, store.eraseCalls);
    TEST_ASSERT_EQUAL_INT(0, store.commitCalls);
}

void test_erase_failure_does_not_commit() {
    MemoryLegacyConfigStore store;
    store.eraseResult = LegacyKeyEraseResult::Error;

    const LegacyConfigMigrationResult result = migrateLegacyAccessCredential(store);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(LegacyConfigMigrationResult::Error),
                          static_cast<int>(result));
    TEST_ASSERT_EQUAL_INT(0, store.commitCalls);
}

void test_commit_failure_is_reported() {
    MemoryLegacyConfigStore store;
    store.eraseResult = LegacyKeyEraseResult::Removed;
    store.commitResult = false;

    const LegacyConfigMigrationResult result = migrateLegacyAccessCredential(store);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(LegacyConfigMigrationResult::Error),
                          static_cast<int>(result));
    TEST_ASSERT_EQUAL_INT(1, store.commitCalls);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_existing_admin_token_is_removed_and_committed);
    RUN_TEST(test_missing_admin_token_does_not_commit);
    RUN_TEST(test_erase_failure_does_not_commit);
    RUN_TEST(test_commit_failure_is_reported);
    UNITY_END();
}

void loop() {}
