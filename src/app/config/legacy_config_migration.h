#pragma once

enum class LegacyKeyEraseResult {
    Removed,
    NotFound,
    Error,
};

class ILegacyConfigStore {
public:
    virtual ~ILegacyConfigStore() = default;
    virtual LegacyKeyEraseResult eraseKey(const char *key) = 0;
    virtual bool commit() = 0;
};

enum class LegacyConfigMigrationResult {
    Removed,
    NotPresent,
    Error,
};

LegacyConfigMigrationResult migrateLegacyAccessCredential(ILegacyConfigStore &store);
