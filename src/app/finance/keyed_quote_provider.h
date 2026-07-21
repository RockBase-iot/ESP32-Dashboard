#pragma once

#include <string>

#include "app/finance/finance_models.h"
#include "app/provider/provider.h"

FetchResult mapFinanceHttpStatus(const std::string &providerId, int httpStatus,
                                 const std::string &retryAfter,
                                 const std::string &payload, int64_t nowUtc = 0);
FinanceQuoteSet parseKeyedQuoteJson(const std::string &json, const std::string &source,
                                    bool delayed, int64_t fallbackAsOfUtc);

class KeyedQuoteProvider final : public IProvider {
public:
    KeyedQuoteProvider(std::string providerId, std::string endpoint)
        : _providerId(std::move(providerId)), _endpoint(std::move(endpoint)) {}

    const char *id() const override { return _providerId.c_str(); }
    uint32_t ttlSeconds() const override { return 15UL * 60UL; }
    FetchResult fetch(const FetchContext &context) override;

private:
    std::string _providerId;
    std::string _endpoint;
};
