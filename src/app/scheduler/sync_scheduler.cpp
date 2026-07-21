#include "sync_scheduler.h"

#include <unordered_set>

bool SyncScheduler::isDue(const ProviderSyncState &state, int64_t nowUtc) const {
    if (!state.enabled || state.providerId.empty()) {
        return false;
    }
    if (state.retryAfterUtc > nowUtc) {
        return false;
    }
    if (state.lastSuccessUtc <= 0) {
        return true;
    }
    return nowUtc - state.lastSuccessUtc >= static_cast<int64_t>(state.ttlSeconds);
}

std::vector<std::string> SyncScheduler::selectDueProviders(
    const std::vector<ProviderSyncState> &states,
    const std::vector<std::string> &referencedProviderIds,
    int64_t nowUtc) const {
    const std::unordered_set<std::string> referenced(referencedProviderIds.begin(),
                                                     referencedProviderIds.end());
    std::vector<std::string> due;
    for (const ProviderSyncState &state : states) {
        if (referenced.find(state.providerId) == referenced.end()) {
            continue;
        }
        if (isDue(state, nowUtc)) {
            due.push_back(state.providerId);
        }
    }
    return due;
}
