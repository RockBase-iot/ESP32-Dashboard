#include "provider_registry.h"

#include <unordered_set>

bool ProviderRegistry::add(IProvider &provider) {
    const char *providerId = provider.id();
    if (providerId == nullptr || providerId[0] == '\0') {
        return false;
    }
    if (find(providerId) != nullptr) {
        return false;
    }
    _providers.push_back(&provider);
    return true;
}

IProvider *ProviderRegistry::find(const std::string &providerId) const {
    for (IProvider *provider : _providers) {
        if (provider != nullptr && providerId == provider->id()) {
            return provider;
        }
    }
    return nullptr;
}

std::vector<FetchResult> ProviderRegistry::fetch(
    const FetchContext &context, const std::vector<std::string> &providerIds) const {
    std::vector<FetchResult> results;
    std::unordered_set<std::string> fetched;
    for (const std::string &providerId : providerIds) {
        if (!fetched.insert(providerId).second) {
            continue;
        }
        IProvider *provider = find(providerId);
        if (provider == nullptr) {
            continue;
        }
        FetchResult result = provider->fetch(context);
        if (result.providerId.empty()) {
            result.providerId = providerId;
        }
        results.push_back(result);
    }
    return results;
}
