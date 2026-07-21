#include "ics_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

namespace {
struct ParsedProperty {
    std::string name;
    std::map<std::string, std::string> params;
    std::string value;
};

struct DateTimeParse {
    bool ok = false;
    bool allDay = false;
    bool floating = false;
    std::string tzid;
    int64_t utc = 0;
};

std::string upperAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

bool parseProperty(const std::string &line, ParsedProperty &out) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
        return false;
    }
    const std::string head = line.substr(0, colon);
    out.value = line.substr(colon + 1);
    size_t start = 0;
    bool first = true;
    while (start <= head.size()) {
        const size_t semi = head.find(';', start);
        const size_t end = semi == std::string::npos ? head.size() : semi;
        const std::string token = head.substr(start, end - start);
        if (first) {
            out.name = upperAscii(token);
            first = false;
        } else {
            const size_t eq = token.find('=');
            if (eq != std::string::npos) {
                out.params[upperAscii(token.substr(0, eq))] = token.substr(eq + 1);
            }
        }
        if (semi == std::string::npos) {
            break;
        }
        start = semi + 1;
    }
    return !out.name.empty();
}

bool isLeap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int daysBeforeMonth(int year, int month) {
    static const int common[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int days = common[month - 1];
    if (month > 2 && isLeap(year)) {
        ++days;
    }
    return days;
}

int64_t daysBeforeYear(int year) {
    int64_t days = 0;
    for (int y = 1970; y < year; ++y) {
        days += isLeap(y) ? 366 : 365;
    }
    return days;
}

bool parseNDigits(const std::string &text, size_t pos, size_t len, int &out) {
    if (pos + len > text.size()) {
        return false;
    }
    int value = 0;
    for (size_t i = 0; i < len; ++i) {
        const char c = text[pos + i];
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
    }
    out = value;
    return true;
}

int64_t epochUtc(int year, int month, int day, int hour, int minute, int second) {
    const int64_t days = daysBeforeYear(year) + daysBeforeMonth(year, month) + (day - 1);
    return days * 86400LL + hour * 3600LL + minute * 60LL + second;
}

DateTimeParse parseDateTimeValue(const ParsedProperty &prop) {
    DateTimeParse result;
    const auto valueParam = prop.params.find("VALUE");
    const bool dateOnly = valueParam != prop.params.end() && upperAscii(valueParam->second) == "DATE";
    const auto tzid = prop.params.find("TZID");
    if (tzid != prop.params.end()) {
        result.tzid = tzid->second;
        result.floating = true;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    if (!parseNDigits(prop.value, 0, 4, year) || !parseNDigits(prop.value, 4, 2, month) ||
        !parseNDigits(prop.value, 6, 2, day)) {
        return result;
    }
    if (dateOnly || prop.value.size() == 8) {
        result.ok = true;
        result.allDay = true;
        result.utc = epochUtc(year, month, day, 0, 0, 0);
        return result;
    }
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (prop.value.size() < 15 || prop.value[8] != 'T' ||
        !parseNDigits(prop.value, 9, 2, hour) ||
        !parseNDigits(prop.value, 11, 2, minute) ||
        !parseNDigits(prop.value, 13, 2, second)) {
        return result;
    }
    result.ok = true;
    if (prop.value.empty() || prop.value.back() != 'Z') {
        result.floating = true;
    }
    result.utc = epochUtc(year, month, day, hour, minute, second);
    return result;
}

int64_t parseDurationSeconds(const std::string &value) {
    bool inTime = false;
    int64_t total = 0;
    int number = 0;
    bool haveNumber = false;
    for (char c : value) {
        if (c == 'P') {
            continue;
        }
        if (c == 'T') {
            inTime = true;
            continue;
        }
        if (c >= '0' && c <= '9') {
            number = number * 10 + (c - '0');
            haveNumber = true;
            continue;
        }
        if (!haveNumber) {
            continue;
        }
        if (c == 'D') total += static_cast<int64_t>(number) * 86400LL;
        if (inTime && c == 'H') total += static_cast<int64_t>(number) * 3600LL;
        if (inTime && c == 'M') total += static_cast<int64_t>(number) * 60LL;
        if (inTime && c == 'S') total += number;
        number = 0;
        haveNumber = false;
    }
    return total;
}

CalendarEventStatus parseStatus(const std::string &value) {
    const std::string upper = upperAscii(value);
    if (upper == "CONFIRMED") return CalendarEventStatus::Confirmed;
    if (upper == "TENTATIVE") return CalendarEventStatus::Tentative;
    if (upper == "CANCELLED") return CalendarEventStatus::Cancelled;
    return CalendarEventStatus::Unknown;
}

bool validEvent(const CalendarEvent &event) {
    return !event.uid.empty() && event.startUtc > 0;
}
}  // namespace

IcsParseResult IcsParser::parse(const std::string &sourceId, IcsByteReader &reader,
                                IcsEventSink &sink,
                                const IcsParserOptions &options) const {
    IcsParseResult result;
    IcsLineReader lines(reader);
    bool inCalendar = false;
    bool inEvent = false;
    bool skipUntilEventEnd = false;
    bool inAlarm = false;
    CalendarEvent event;
    int64_t durationSeconds = 0;

    while (true) {
        const IcsLogicalLine line = lines.next();
        result.byteCount = lines.bytesRead();
        if (line.status == IcsLineReadStatus::End) {
            break;
        }
        if (line.status != IcsLineReadStatus::Ok) {
            if (inEvent) {
                skipUntilEventEnd = true;
                continue;
            }
            result.state = SourceState::Parse;
            return result;
        }

        ParsedProperty prop;
        if (!parseProperty(line.text, prop)) {
            if (inEvent) {
                skipUntilEventEnd = true;
                continue;
            }
            result.state = SourceState::Parse;
            return result;
        }
        if (prop.name == "BEGIN" && upperAscii(prop.value) == "VCALENDAR") {
            inCalendar = true;
            continue;
        }
        if (!inCalendar) {
            result.state = SourceState::Parse;
            return result;
        }
        if (prop.name == "BEGIN" && upperAscii(prop.value) == "VEVENT") {
            inEvent = true;
            skipUntilEventEnd = false;
            inAlarm = false;
            event = CalendarEvent{};
            event.sourceId = sourceId;
            durationSeconds = 0;
            continue;
        }
        if (!inEvent) {
            continue;
        }
        if (prop.name == "BEGIN" && upperAscii(prop.value) == "VALARM") {
            inAlarm = true;
            continue;
        }
        if (prop.name == "END" && upperAscii(prop.value) == "VALARM") {
            inAlarm = false;
            continue;
        }
        if (prop.name == "END" && upperAscii(prop.value) == "VEVENT") {
            if (durationSeconds > 0 && event.endUtc == 0) {
                event.endUtc = event.startUtc + durationSeconds;
            }
            if (event.allDay && event.endUtc == 0) {
                event.endUtc = event.startUtc + 86400LL;
            }
            if (!skipUntilEventEnd && validEvent(event) && sink.onEvent(event)) {
                ++result.eventCount;
            } else {
                ++result.skippedCount;
            }
            inEvent = false;
            skipUntilEventEnd = false;
            inAlarm = false;
            continue;
        }
        if (skipUntilEventEnd || inAlarm) {
            continue;
        }

        if (prop.name == "UID") {
            event.uid = truncateCalendarUtf8(prop.value, 128);
        } else if (prop.name == "SUMMARY") {
            event.summary = truncateCalendarUtf8(unescapeIcsText(prop.value), options.maxSummaryBytes);
        } else if (prop.name == "LOCATION") {
            event.location = truncateCalendarUtf8(unescapeIcsText(prop.value), options.maxLocationBytes);
        } else if (prop.name == "DESCRIPTION") {
            event.description = truncateCalendarUtf8(unescapeIcsText(prop.value),
                                                     options.maxDescriptionBytes);
        } else if (prop.name == "DTSTART") {
            const DateTimeParse parsed = parseDateTimeValue(prop);
            if (!parsed.ok) {
                skipUntilEventEnd = true;
                continue;
            }
            event.startUtc = parsed.utc;
            event.allDay = parsed.allDay;
            event.startFloating = parsed.floating;
            event.startTzid = parsed.tzid;
        } else if (prop.name == "DTEND") {
            const DateTimeParse parsed = parseDateTimeValue(prop);
            if (!parsed.ok) {
                skipUntilEventEnd = true;
                continue;
            }
            event.endUtc = parsed.utc;
            event.endFloating = parsed.floating;
            event.endTzid = parsed.tzid;
        } else if (prop.name == "DURATION") {
            durationSeconds = parseDurationSeconds(prop.value);
        } else if (prop.name == "DTSTAMP") {
            event.dtstampUtc = parseDateTimeValue(prop).utc;
        } else if (prop.name == "SEQUENCE") {
            event.sequence = static_cast<uint32_t>(std::strtoul(prop.value.c_str(), nullptr, 10));
        } else if (prop.name == "STATUS") {
            event.status = parseStatus(prop.value);
        } else if (prop.name == "RECURRENCE-ID") {
            event.recurrenceId = prop.value;
        } else if (prop.name == "RRULE") {
            event.rrule = prop.value;
        } else if (prop.name == "RDATE") {
            event.rdate.push_back(prop.value);
        } else if (prop.name == "EXDATE") {
            event.exdate.push_back(prop.value);
        }
    }

    if (!inCalendar) {
        result.state = SourceState::Parse;
    }
    return result;
}
