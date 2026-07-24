#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "app/finance/finance_models.h"

inline bool isValidPortfolioPosition(const PortfolioPosition &position) {
    return !position.ticker.empty() && position.quantity >= 0.0f &&
           position.costBasis >= 0.0f && !position.currency.empty();
}

inline std::string trimPortfolioToken(const std::string &text) {
    size_t begin = 0;
    while (begin < text.size() && (text[begin] == ' ' || text[begin] == '\t')) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t')) {
        --end;
    }
    return text.substr(begin, end - begin);
}

inline std::vector<PortfolioPosition> parsePortfolioPositionsText(const std::string &text) {
    std::vector<PortfolioPosition> positions;
    std::string row;
    auto flush = [&]() {
        row = trimPortfolioToken(row);
        if (row.empty()) {
            return;
        }
        std::vector<std::string> fields;
        std::string field;
        for (char c : row) {
            if (c == ':') {
                fields.push_back(trimPortfolioToken(field));
                field.clear();
            } else {
                field.push_back(c);
            }
        }
        fields.push_back(trimPortfolioToken(field));
        if (fields.size() != 4) {
            return;
        }
        PortfolioPosition position;
        position.ticker = fields[0];
        position.quantity = std::strtof(fields[1].c_str(), nullptr);
        position.costBasis = std::strtof(fields[2].c_str(), nullptr);
        position.currency = fields[3];
        if (isValidPortfolioPosition(position)) {
            positions.push_back(position);
        }
    };

    for (char c : text) {
        if (c == ',' || c == '\n' || c == '\r' || c == ';') {
            flush();
            row.clear();
        } else {
            row.push_back(c);
        }
    }
    flush();
    return positions;
}

inline bool portfolioTickerMatchesQuote(const std::string &positionTicker,
                                        const std::string &quoteTicker) {
    if (positionTicker == quoteTicker) {
        return true;
    }
    if (positionTicker.empty() || quoteTicker.size() <= positionTicker.size()) {
        return false;
    }
    return quoteTicker.compare(0, positionTicker.size(), positionTicker) == 0 &&
           quoteTicker[positionTicker.size()] == '.';
}

inline PortfolioSummary calculatePortfolioSummary(const PortfolioSnapshot &snapshot) {
    PortfolioSummary summary;
    for (const PortfolioPosition &position : snapshot.positions) {
        if (!isValidPortfolioPosition(position)) {
            continue;
        }
        if (summary.currency.empty() || summary.currency == "USD") {
            summary.currency = position.currency;
        }
        const FinanceQuote *quote = nullptr;
        for (const FinanceQuote &candidate : snapshot.quotes) {
            if (portfolioTickerMatchesQuote(position.ticker, candidate.ticker) &&
                isValidFinanceQuote(candidate)) {
                quote = &candidate;
                break;
            }
        }
        if (quote == nullptr || quote->currency != position.currency) {
            ++summary.missingQuotes;
            continue;
        }

        PortfolioHolding holding;
        holding.ticker = position.ticker;
        holding.marketValue = position.quantity * quote->price;
        holding.costBasis = position.quantity * position.costBasis;
        holding.gainLoss = holding.marketValue - holding.costBasis;
        holding.gainLossPercent =
            holding.costBasis > 0.0f ? (holding.gainLoss / holding.costBasis) * 100.0f : 0.0f;

        summary.marketValue += holding.marketValue;
        summary.costBasis += holding.costBasis;
        summary.holdings.push_back(holding);
        summary.topMovers.push_back(holding);
    }

    summary.gainLoss = summary.marketValue - summary.costBasis;
    summary.gainLossPercent =
        summary.costBasis > 0.0f ? (summary.gainLoss / summary.costBasis) * 100.0f : 0.0f;

    std::sort(summary.topMovers.begin(), summary.topMovers.end(),
              [](const PortfolioHolding &lhs, const PortfolioHolding &rhs) {
                  return std::fabs(lhs.gainLossPercent) > std::fabs(rhs.gainLossPercent);
              });
    if (summary.topMovers.size() > 3) {
        summary.topMovers.resize(3);
    }
    return summary;
}

inline std::string exportPortfolioConfig(const PortfolioSnapshot &snapshot, bool includePositions) {
    if (!includePositions) {
        return "{\"portfolio\":\"excluded\"}";
    }
    auto escapeJson = [](const std::string &text) {
        std::string out;
        for (char c : text) {
            if (c == '"' || c == '\\') {
                out.push_back('\\');
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                continue;
            }
            out.push_back(c);
        }
        return out;
    };
    std::string json = "{\"positions\":[";
    for (size_t i = 0; i < snapshot.positions.size(); ++i) {
        const PortfolioPosition &position = snapshot.positions[i];
        if (i > 0) {
            json += ",";
        }
        json += "{\"ticker\":\"" + escapeJson(position.ticker) + "\"}";
    }
    json += "]}";
    return json;
}
