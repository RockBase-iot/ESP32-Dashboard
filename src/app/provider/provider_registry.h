#pragma once

#include <string>
#include <vector>

#include "app/provider/provider.h"

class ProviderRegistry {
public:
    bool add(IProvider &provider);
    IProvider *find(const std::string &providerId) const;
    std::vector<FetchResult> fetch(const FetchContext &context,
                                   const std::vector<std::string> &providerIds) const;

private:
    std::vector<IProvider *> _providers;
};
