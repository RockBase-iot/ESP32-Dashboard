#pragma once

#include <string>

#include "app/provider/provider.h"

class KeyedNewsProvider final : public IProvider {
public:
    KeyedNewsProvider(std::string providerId, std::string endpoint)
        : _providerId(std::move(providerId)), _endpoint(std::move(endpoint)) {}

    const char *id() const override { return _providerId.c_str(); }
    uint32_t ttlSeconds() const override { return 6UL * 3600UL; }
    FetchResult fetch(const FetchContext &context) override;

private:
    std::string _providerId;
    std::string _endpoint;
};
