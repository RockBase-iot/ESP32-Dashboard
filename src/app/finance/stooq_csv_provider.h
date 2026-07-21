#pragma once

#include <string>

#include "app/finance/finance_models.h"
#include "app/provider/provider.h"

FinanceQuoteSet parseStooqCsvQuotes(const std::string &csv, const std::string &source,
                                    int64_t fallbackAsOfUtc);

class StooqCsvProvider final : public IProvider {
public:
    StooqCsvProvider(std::string providerId, std::string symbolsCsv)
        : _providerId(std::move(providerId)), _symbolsCsv(std::move(symbolsCsv)) {}

    const char *id() const override { return _providerId.c_str(); }
    uint32_t ttlSeconds() const override { return 15UL * 60UL; }
    FetchResult fetch(const FetchContext &context) override;

private:
    std::string _providerId;
    std::string _symbolsCsv;
};
