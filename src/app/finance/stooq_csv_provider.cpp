#include "stooq_csv_provider.h"

#include <cerrno>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace {
std::vector<std::string> splitFinanceCsvLine(const std::string &line) {
    std::vector<std::string> fields;
    std::string current;
    bool quoted = false;
    for (char c : line) {
        if (c == '"') {
            quoted = !quoted;
        } else if (c == ',' && !quoted) {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current.push_back(c);
        }
    }
    fields.push_back(current);
    return fields;
}

int64_t daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

int64_t parseFinanceUtcTimestamp(const std::string &date, const std::string &time) {
    if (date.size() < 10 || time.size() < 5) {
        return 0;
    }
    const int year = std::atoi(date.substr(0, 4).c_str());
    const unsigned month = static_cast<unsigned>(std::atoi(date.substr(5, 2).c_str()));
    const unsigned day = static_cast<unsigned>(std::atoi(date.substr(8, 2).c_str()));
    const int hour = std::atoi(time.substr(0, 2).c_str());
    const int minute = std::atoi(time.substr(3, 2).c_str());
    const int second = time.size() >= 8 ? std::atoi(time.substr(6, 2).c_str()) : 0;
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return 0;
    }
    return daysFromCivil(year, month, day) * 86400LL + hour * 3600LL + minute * 60LL + second;
}

bool parseStrictFloat(const std::string &text, float &out) {
    if (text.empty()) {
        return false;
    }
    char *end = nullptr;
    errno = 0;
    const float value = std::strtof(text.c_str(), &end);
    if (errno != 0 || end == text.c_str() || (end != nullptr && *end != '\0')) {
        return false;
    }
    out = value;
    return true;
}

std::string trimFinanceText(const std::string &value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::vector<std::string> splitSymbolText(const std::string &symbolsText) {
    std::vector<std::string> symbols;
    std::string token;
    for (char c : symbolsText) {
        if (c == ',' || c == '\n' || c == '\r' || c == ';') {
            token = trimFinanceText(token);
            if (!token.empty()) {
                std::transform(token.begin(), token.end(), token.begin(), [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });
                symbols.push_back(token);
            }
            token.clear();
        } else {
            token.push_back(c);
        }
    }
    token = trimFinanceText(token);
    if (!token.empty()) {
        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        symbols.push_back(token);
    }
    return symbols;
}
}  // namespace

std::string buildStooqCsvUrlForHost(const std::string &host, const std::string &symbolsText) {
    const auto symbols = splitSymbolText(symbolsText);
    const std::string cleanHost = trimFinanceText(host);
    if (symbols.empty() || cleanHost.empty()) {
        return "";
    }
    std::string joined;
    for (size_t i = 0; i < symbols.size(); ++i) {
        if (i > 0) {
            joined += ",";
        }
        joined += symbols[i];
    }
    return "https://" + cleanHost + "/q/l/?s=" + joined + "&f=sd2t2ohlcv&h&e=csv";
}

std::string buildStooqCsvUrl(const std::string &symbolsText) {
    return buildStooqCsvUrlForHost("stooq.com", symbolsText);
}

FinanceQuoteSet parseStooqCsvQuotes(const std::string &csv, const std::string &source,
                                    int64_t fallbackAsOfUtc) {
    FinanceQuoteSet parsed;
    if (source.empty()) {
        parsed.rejectedRows = 1;
        return parsed;
    }

    std::istringstream input(csv);
    std::string line;
    bool headerSeen = false;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        if (!headerSeen) {
            headerSeen = true;
            if (line.find("Symbol") != std::string::npos || line.find("Close") != std::string::npos) {
                continue;
            }
        }
        const auto fields = splitFinanceCsvLine(line);
        if (fields.size() < 7) {
            ++parsed.rejectedRows;
            parsed.partialFailure = true;
            continue;
        }

        FinanceQuote quote;
        quote.ticker = fields[0];
        float open = 0.0f;
        if (!parseStrictFloat(fields[6], quote.price) || !parseStrictFloat(fields[3], open)) {
            ++parsed.rejectedRows;
            parsed.partialFailure = true;
            continue;
        }
        quote.change = quote.price - open;
        quote.changePercent = open > 0.0f ? (quote.change / open) * 100.0f : 0.0f;
        quote.currency = "USD";
        quote.asOfUtc = parseFinanceUtcTimestamp(fields[1], fields[2]);
        quote.delayed = true;
        quote.source = source;

        (void)fallbackAsOfUtc;
        if (!isValidFinanceQuote(quote)) {
            ++parsed.rejectedRows;
            parsed.partialFailure = true;
            continue;
        }
        parsed.quotes.push_back(quote);
    }
    return parsed;
}

FetchResult StooqCsvProvider::fetch(const FetchContext &context) {
    FetchResult result;
    result.providerId = _providerId;
    result.state = _symbolsCsv.empty() ? SourceState::NotFound : SourceState::Stale;
    result.changed = false;
    result.retryAfterUtc = 0;
    result.itemCount = 0;
    (void)context;
    return result;
}
