#include "keyed_quote_provider.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <ArduinoJson.h>

namespace {
SourceState financeStateFromHttp(int status) {
    if (status == 200 || status == 304) {
        return SourceState::Ok;
    }
    if (status == 401 || status == 403) {
        return SourceState::Auth;
    }
    if (status == 404) {
        return SourceState::NotFound;
    }
    if (status == 429) {
        return SourceState::RateLimit;
    }
    return SourceState::Stale;
}

int64_t keyedQuoteDaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

int monthFromHttpDate(const char *month) {
    const std::string text(month == nullptr ? "" : month);
    if (text == "Jan") return 1;
    if (text == "Feb") return 2;
    if (text == "Mar") return 3;
    if (text == "Apr") return 4;
    if (text == "May") return 5;
    if (text == "Jun") return 6;
    if (text == "Jul") return 7;
    if (text == "Aug") return 8;
    if (text == "Sep") return 9;
    if (text == "Oct") return 10;
    if (text == "Nov") return 11;
    if (text == "Dec") return 12;
    return 0;
}

int64_t parseHttpDateUtc(const std::string &text) {
    char weekday[8] = {};
    char monthText[4] = {};
    int day = 0;
    int year = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int matched = std::sscanf(text.c_str(), "%7[^,], %d %3s %d %d:%d:%d", weekday, &day,
                              monthText, &year, &hour, &minute, &second);
    if (matched != 7) {
        matched = std::sscanf(text.c_str(), "%d %3s %d %d:%d:%d", &day, monthText, &year,
                              &hour, &minute, &second);
        if (matched != 6) {
            return 0;
        }
    }
    const int month = monthFromHttpDate(monthText);
    if (year < 1970 || month < 1 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59) {
        return 0;
    }
    return keyedQuoteDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) *
               86400LL +
           static_cast<int64_t>(hour) * 3600LL + static_cast<int64_t>(minute) * 60LL + second;
}

int64_t parseRetryAfterUtc(const std::string &retryAfter, int64_t nowUtc) {
    if (retryAfter.empty()) {
        return 0;
    }
    const bool deltaSeconds = std::all_of(retryAfter.begin(), retryAfter.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    });
    if (deltaSeconds) {
        const int64_t seconds = std::strtoll(retryAfter.c_str(), nullptr, 10);
        return nowUtc > 0 ? nowUtc + seconds : seconds;
    }
    return parseHttpDateUtc(retryAfter);
}

bool assignRequiredNumber(JsonVariantConst value, float &out) {
    if (value.isNull() || !value.is<float>()) {
        return false;
    }
    out = value.as<float>();
    return true;
}

bool assignRequiredTimestamp(JsonVariantConst value, int64_t &out) {
    if (value.isNull() || !value.is<int64_t>()) {
        return false;
    }
    out = value.as<int64_t>();
    return out > 0;
}

void appendKeyedQuote(FinanceQuoteSet &parsed, JsonObjectConst item, const std::string &source,
                      bool delayed) {
    FinanceQuote quote;
    quote.ticker = item["ticker"] | item["symbol"] | "";
    quote.currency = item["currency"] | "";
    quote.delayed = delayed;
    quote.source = source;
    if (!assignRequiredNumber(item["price"], quote.price) ||
        !assignRequiredNumber(item["change"], quote.change) ||
        !assignRequiredNumber(item["changePercent"].isNull() ? item["change_percent"]
                                                             : item["changePercent"],
                              quote.changePercent) ||
        !assignRequiredTimestamp(item["asOfUtc"], quote.asOfUtc) ||
        !isValidFinanceQuote(quote)) {
        ++parsed.rejectedRows;
        parsed.partialFailure = true;
        return;
    }
    parsed.quotes.push_back(quote);
}
}  // namespace

FetchResult mapFinanceHttpStatus(const std::string &providerId, int httpStatus,
                                 const std::string &retryAfter,
                                 const std::string &payload, int64_t nowUtc) {
    FetchResult result;
    result.providerId = providerId;
    result.state = financeStateFromHttp(httpStatus);
    result.changed = httpStatus == 200 && !payload.empty();
    result.retryAfterUtc = httpStatus == 429 ? parseRetryAfterUtc(retryAfter, nowUtc) : 0;
    if (result.changed) {
        result.payload.assign(payload.begin(), payload.end());
    }
    return result;
}

FinanceQuoteSet parseKeyedQuoteJson(const std::string &json, const std::string &source,
                                    bool delayed, int64_t fallbackAsOfUtc) {
    FinanceQuoteSet parsed;
    (void)fallbackAsOfUtc;
    if (source.empty()) {
        parsed.rejectedRows = 1;
        return parsed;
    }

    JsonDocument doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) {
        parsed.partialFailure = true;
        parsed.rejectedRows = 1;
        return parsed;
    }

    JsonArray quotes = doc["quotes"].as<JsonArray>();
    if (quotes.isNull()) {
        JsonObjectConst single = doc.as<JsonObjectConst>();
        if (!single.isNull()) {
            appendKeyedQuote(parsed, single, source, delayed);
        }
        return parsed;
    }

    for (JsonObjectConst item : quotes) {
        appendKeyedQuote(parsed, item, source, delayed);
    }
    return parsed;
}

FetchResult KeyedQuoteProvider::fetch(const FetchContext & /*context*/) {
    FetchResult result;
    result.providerId = _providerId;
    result.state = _endpoint.empty() ? SourceState::NotFound : SourceState::Stale;
    result.changed = false;
    return result;
}
