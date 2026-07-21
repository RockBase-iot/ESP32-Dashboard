#pragma once

#include <stdint.h>

#include <string>
#include <vector>

struct ProviderSyncState {
    std::string providerId;
    uint32_t ttlSeconds = 0;
    int64_t lastSuccessUtc = 0;
    int64_t retryAfterUtc = 0;
    bool enabled = true;
};

class SyncScheduler {
public:
    bool isDue(const ProviderSyncState &state, int64_t nowUtc) const;
    std::vector<std::string> selectDueProviders(
        const std::vector<ProviderSyncState> &states,
        const std::vector<std::string> &referencedProviderIds,
        int64_t nowUtc) const;
};
