#pragma once

#include <string>

#include "app/finance/finance_models.h"
#include "app/provider/provider.h"

EconomicEventSet parseEconomicRssFeed(const std::string &xml, const std::string &source,
                                      size_t maxEvents);
EconomicEventSet parseEconomicIcsFeed(const std::string &ics, const std::string &source,
                                      size_t maxEvents);

class EconomicFeedProvider final : public IProvider {
public:
    EconomicFeedProvider(std::string providerId, std::string endpoint)
        : _providerId(std::move(providerId)), _endpoint(std::move(endpoint)) {}

    const char *id() const override { return _providerId.c_str(); }
    uint32_t ttlSeconds() const override { return 6UL * 3600UL; }
    FetchResult fetch(const FetchContext &context) override;

private:
    std::string _providerId;
    std::string _endpoint;
};
