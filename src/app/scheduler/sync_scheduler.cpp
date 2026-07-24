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

PageSyncRequirements syncRequirementsForDisplayPage(bool homeWeather, PageId page) {
    PageSyncRequirements requirements;
    if (homeWeather) {
        requirements.weather = true;
        return requirements;
    }

    const PageDescriptor *descriptor = findPage(page);
    if (!descriptor) {
        return requirements;
    }

    for (const std::string &provider : descriptor->requiredProviders) {
        if (provider == "weather") {
            requirements.weather = true;
        } else if (provider == "calendar") {
            requirements.calendar = true;
        } else if (provider == "finance" || provider == "economic") {
            requirements.finance = true;
        } else if (provider == "news" || provider == "history") {
            requirements.news = true;
        }
    }
    return requirements;
}
