#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/page/page_catalog.h"

struct ProviderSyncState {
    std::string providerId;
    uint32_t ttlSeconds = 0;
    int64_t lastSuccessUtc = 0;
    int64_t retryAfterUtc = 0;
    bool enabled = true;
};

struct PageSyncRequirements {
    bool weather = false;
    bool calendar = false;
    bool finance = false;
    bool news = false;
};

class SyncScheduler {
public:
    bool isDue(const ProviderSyncState &state, int64_t nowUtc) const;
    std::vector<std::string> selectDueProviders(
        const std::vector<ProviderSyncState> &states,
        const std::vector<std::string> &referencedProviderIds,
        int64_t nowUtc) const;
};

PageSyncRequirements syncRequirementsForDisplayPage(bool homeWeather, PageId page);
PageSyncRequirements missingSyncRequirements(const PageSyncRequirements &required,
                                             const PageSyncRequirements &completed);
void markSyncRequirementsCompleted(PageSyncRequirements &completed,
                                   const PageSyncRequirements &finished);
bool hasSyncRequirements(const PageSyncRequirements &requirements);
