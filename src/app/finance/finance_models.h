#pragma once

#include <stdint.h>

#include <string>
#include <vector>

enum class FinanceImpact : uint8_t {
    Low,
    Medium,
    High,
};

struct FinanceQuote {
    std::string ticker;
    float price = 0.0f;
    float change = 0.0f;
    float changePercent = 0.0f;
    std::string currency;
    int64_t asOfUtc = 0;
    bool delayed = true;
    std::string source;
};

struct FinanceQuoteSet {
    std::vector<FinanceQuote> quotes;
    uint32_t rejectedRows = 0;
    bool partialFailure = false;
};

struct PortfolioPosition {
    std::string ticker;
    float quantity = 0.0f;
    float costBasis = 0.0f;
    std::string currency;
};

struct PortfolioHolding {
    std::string ticker;
    float marketValue = 0.0f;
    float costBasis = 0.0f;
    float gainLoss = 0.0f;
    float gainLossPercent = 0.0f;
};

struct PortfolioSummary {
    float marketValue = 0.0f;
    float costBasis = 0.0f;
    float gainLoss = 0.0f;
    float gainLossPercent = 0.0f;
    uint32_t missingQuotes = 0;
    std::string currency = "USD";
    std::vector<PortfolioHolding> holdings;
    std::vector<PortfolioHolding> topMovers;
};

struct PortfolioSnapshot {
    std::vector<PortfolioPosition> positions;
    std::vector<FinanceQuote> quotes;
};

struct EconomicEvent {
    int64_t startsAtUtc = 0;
    std::string region;
    FinanceImpact impact = FinanceImpact::Low;
    std::string name;
    std::string forecast;
    std::string previous;
    std::string source;
};

struct EconomicEventSet {
    std::vector<EconomicEvent> events;
    uint32_t rejectedItems = 0;
};

inline bool isValidFinanceQuote(const FinanceQuote &quote) {
    return !quote.ticker.empty() && quote.price >= 0.0f && !quote.currency.empty() &&
           quote.asOfUtc > 0 && !quote.source.empty();
}

inline bool isFinanceQuoteFresh(const FinanceQuote &quote, int64_t nowUtc, uint32_t maxAgeSeconds) {
    if (!isValidFinanceQuote(quote) || nowUtc < quote.asOfUtc) {
        return false;
    }
    return static_cast<uint64_t>(nowUtc - quote.asOfUtc) <= maxAgeSeconds;
}

inline const char *financeImpactLabel(FinanceImpact impact) {
    switch (impact) {
        case FinanceImpact::High:
            return "High";
        case FinanceImpact::Medium:
            return "Medium";
        case FinanceImpact::Low:
        default:
            return "Low";
    }
}
