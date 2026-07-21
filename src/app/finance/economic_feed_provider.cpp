#include "economic_feed_provider.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace {
std::string financeLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string extractBetween(const std::string &text, const std::string &open,
                           const std::string &close, size_t start = 0) {
    const size_t from = text.find(open, start);
    if (from == std::string::npos) {
        return "";
    }
    const size_t body = from + open.size();
    const size_t to = text.find(close, body);
    if (to == std::string::npos) {
        return "";
    }
    return text.substr(body, to - body);
}

std::string extractIcsValue(const std::string &block, const std::string &key) {
    const size_t pos = block.find(key + ":");
    if (pos == std::string::npos) {
        return "";
    }
    const size_t body = pos + key.size() + 1;
    const size_t end = block.find_first_of("\r\n", body);
    return block.substr(body, end == std::string::npos ? std::string::npos : end - body);
}

int64_t economicDaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

bool parseEconomicDigits(const std::string &digits, size_t pos, size_t length, int &out) {
    if (pos + length > digits.size()) {
        return false;
    }
    int value = 0;
    for (size_t i = 0; i < length; ++i) {
        const char c = digits[pos + i];
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
    }
    out = value;
    return true;
}

int monthFromRssDate(const char *month) {
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

int64_t parseRssDateUtc(const std::string &text) {
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
    const int month = monthFromRssDate(monthText);
    if (year < 1970 || month < 1 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59) {
        return 0;
    }
    return economicDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) *
               86400LL +
           static_cast<int64_t>(hour) * 3600LL + static_cast<int64_t>(minute) * 60LL + second;
}

int64_t parseEconomicUtcTimestamp(const std::string &text) {
    const int64_t rssDate = parseRssDateUtc(text);
    if (rssDate > 0) {
        return rssDate;
    }
    std::string digits;
    for (char c : text) {
        if (c >= '0' && c <= '9') {
            digits.push_back(c);
        }
    }
    if (digits.size() < 8) {
        return 0;
    }
    if (digits.size() > 14) {
        digits.resize(14);
    }
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!parseEconomicDigits(digits, 0, 4, year) ||
        !parseEconomicDigits(digits, 4, 2, month) ||
        !parseEconomicDigits(digits, 6, 2, day)) {
        return 0;
    }
    if (digits.size() >= 14) {
        if (!parseEconomicDigits(digits, 8, 2, hour) ||
            !parseEconomicDigits(digits, 10, 2, minute) ||
            !parseEconomicDigits(digits, 12, 2, second)) {
            return 0;
        }
    }
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return 0;
    }
    return economicDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) *
               86400LL +
           static_cast<int64_t>(hour) * 3600LL + static_cast<int64_t>(minute) * 60LL + second;
}

FinanceImpact inferImpact(const std::string &title) {
    const std::string lower = financeLowerAscii(title);
    if (lower.find("high") != std::string::npos || lower.find("cpi") != std::string::npos ||
        lower.find("rate") != std::string::npos) {
        return FinanceImpact::High;
    }
    if (lower.find("medium") != std::string::npos || lower.find("gdp") != std::string::npos ||
        lower.find("pmi") != std::string::npos) {
        return FinanceImpact::Medium;
    }
    return FinanceImpact::Low;
}

std::string inferRegion(const std::string &title) {
    const std::string lower = financeLowerAscii(title);
    if (lower.find("us") != std::string::npos || lower.find("fed") != std::string::npos) {
        return "US";
    }
    if (lower.find("eu") != std::string::npos || lower.find("ecb") != std::string::npos) {
        return "EU";
    }
    if (lower.find("uk") != std::string::npos) {
        return "UK";
    }
    if (lower.find("cn") != std::string::npos || lower.find("china") != std::string::npos) {
        return "CN";
    }
    return "GLOBAL";
}

bool addEconomicEvent(EconomicEventSet &set, EconomicEvent event, size_t maxEvents) {
    if (set.events.size() >= maxEvents) {
        return false;
    }
    if (event.name.empty() || event.source.empty() || event.startsAtUtc <= 0) {
        ++set.rejectedItems;
        return false;
    }
    event.region = event.region.empty() ? inferRegion(event.name) : event.region;
    event.impact = inferImpact(event.name);
    set.events.push_back(event);
    return true;
}
}  // namespace

EconomicEventSet parseEconomicRssFeed(const std::string &xml, const std::string &source,
                                      size_t maxEvents) {
    EconomicEventSet set;
    if (source.empty()) {
        set.rejectedItems = 1;
        return set;
    }

    size_t pos = 0;
    while (set.events.size() < maxEvents) {
        const size_t itemStart = xml.find("<item", pos);
        if (itemStart == std::string::npos) {
            break;
        }
        const size_t itemEnd = xml.find("</item>", itemStart);
        if (itemEnd == std::string::npos) {
            ++set.rejectedItems;
            break;
        }
        const std::string item = xml.substr(itemStart, itemEnd - itemStart);
        EconomicEvent event;
        event.name = extractBetween(item, "<title>", "</title>");
        event.startsAtUtc = parseEconomicUtcTimestamp(extractBetween(item, "<pubDate>", "</pubDate>"));
        event.forecast = extractBetween(item, "<forecast>", "</forecast>");
        event.previous = extractBetween(item, "<previous>", "</previous>");
        event.source = extractBetween(item, "<source>", "</source>");
        if (event.source.empty()) {
            event.source = source;
        }
        addEconomicEvent(set, event, maxEvents);
        pos = itemEnd + 7;
    }
    return set;
}

EconomicEventSet parseEconomicIcsFeed(const std::string &ics, const std::string &source,
                                      size_t maxEvents) {
    EconomicEventSet set;
    if (source.empty()) {
        set.rejectedItems = 1;
        return set;
    }

    size_t pos = 0;
    while (set.events.size() < maxEvents) {
        const size_t start = ics.find("BEGIN:VEVENT", pos);
        if (start == std::string::npos) {
            break;
        }
        const size_t end = ics.find("END:VEVENT", start);
        if (end == std::string::npos) {
            ++set.rejectedItems;
            break;
        }
        const std::string block = ics.substr(start, end - start);
        EconomicEvent event;
        event.name = extractIcsValue(block, "SUMMARY");
        event.startsAtUtc = parseEconomicUtcTimestamp(extractIcsValue(block, "DTSTART"));
        event.forecast = extractIcsValue(block, "X-FORECAST");
        event.previous = extractIcsValue(block, "X-PREVIOUS");
        if (event.forecast.empty()) {
            event.forecast = extractIcsValue(block, "FORECAST");
        }
        if (event.previous.empty()) {
            event.previous = extractIcsValue(block, "PREVIOUS");
        }
        event.source = source;
        addEconomicEvent(set, event, maxEvents);
        pos = end + 10;
    }
    return set;
}

FetchResult EconomicFeedProvider::fetch(const FetchContext & /*context*/) {
    FetchResult result;
    result.providerId = _providerId;
    result.state = _endpoint.empty() ? SourceState::NotFound : SourceState::Stale;
    result.changed = false;
    return result;
}
