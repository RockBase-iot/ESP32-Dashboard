#include "legacy_config_migration.h"

namespace {
constexpr const char *kLegacyAdminTokenKey = "AdminTokH";
}

LegacyConfigMigrationResult migrateLegacyAccessCredential(ILegacyConfigStore &store) {
    switch (store.eraseKey(kLegacyAdminTokenKey)) {
        case LegacyKeyEraseResult::Removed:
            return store.commit() ? LegacyConfigMigrationResult::Removed
                                  : LegacyConfigMigrationResult::Error;
        case LegacyKeyEraseResult::NotFound:
            return LegacyConfigMigrationResult::NotPresent;
        case LegacyKeyEraseResult::Error:
            return LegacyConfigMigrationResult::Error;
    }
    return LegacyConfigMigrationResult::Error;
}
