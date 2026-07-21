#pragma once

#include <string>

#include "app/provider/provider.h"

class WikimediaHistoryProvider final : public IProvider {
public:
    const char *id() const override { return "history"; }
    uint32_t ttlSeconds() const override { return 24UL * 3600UL; }
    FetchResult fetch(const FetchContext &context) override;
};
