#include "recurrence_rule.h"

#include <algorithm>
#include <cctype>

namespace {
int64_t recurrenceRuleDaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

void recurrenceRuleCivilFromDays(int64_t z, int &year, unsigned &month, unsigned &day) {
    z += 719468LL;
    const int era = static_cast<int>((z >= 0 ? z : z - 146096) / 146097);
    const unsigned doe = static_cast<unsigned>(z - static_cast<int64_t>(era) * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    year = static_cast<int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    day = doy - (153 * mp + 2) / 5 + 1;
    month = mp + (mp < 10 ? 3 : static_cast<unsigned>(-9));
    year += (month <= 2);
}

std::string upperAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

std::vector<std::string> split(const std::string &text, char delimiter) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t pos = text.find(delimiter, start);
        const size_t end = pos == std::string::npos ? text.size() : pos;
        out.push_back(text.substr(start, end - start));
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    return out;
}

bool parseDigits(const std::string &text, size_t pos, size_t length, int &out) {
    if (pos + length > text.size()) {
        return false;
    }
    int value = 0;
    for (size_t i = 0; i < length; ++i) {
        const char c = text[pos + i];
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
    }
    out = value;
    return true;
}

}  // namespace

bool parseRecurrenceRule(const std::string &text, RecurrenceRule &rule) {
    RecurrenceRule parsed;
    const std::vector<std::string> parts = split(text, ';');
    for (const std::string &part : parts) {
        const size_t eq = part.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = upperAscii(part.substr(0, eq));
        const std::string value = upperAscii(part.substr(eq + 1));
        if (key == "FREQ") {
            if (value == "DAILY") parsed.frequency = RecurrenceFrequency::Daily;
            else if (value == "WEEKLY") parsed.frequency = RecurrenceFrequency::Weekly;
            else if (value == "MONTHLY") parsed.frequency = RecurrenceFrequency::Monthly;
            else if (value == "YEARLY") parsed.frequency = RecurrenceFrequency::Yearly;
        } else if (key == "COUNT") {
            parsed.count = std::max(0, std::atoi(value.c_str()));
        } else if (key == "INTERVAL") {
            parsed.interval = std::max(1, std::atoi(value.c_str()));
        } else if (key == "UNTIL") {
            int64_t utc = 0;
            bool allDay = false;
            if (parseIcsDateTimeUtc(value, utc, allDay)) {
                parsed.untilUtc = utc + (allDay ? 86399LL : 0LL);
            }
        } else if (key == "BYDAY") {
            for (const std::string &token : split(value, ',')) {
                std::string day = token;
                if (day.size() > 2) {
                    day = day.substr(day.size() - 2);
                }
                if (day == "SU") parsed.byWeekdays.push_back(0);
                else if (day == "MO") parsed.byWeekdays.push_back(1);
                else if (day == "TU") parsed.byWeekdays.push_back(2);
                else if (day == "WE") parsed.byWeekdays.push_back(3);
                else if (day == "TH") parsed.byWeekdays.push_back(4);
                else if (day == "FR") parsed.byWeekdays.push_back(5);
                else if (day == "SA") parsed.byWeekdays.push_back(6);
            }
        } else if (key == "BYMONTHDAY") {
            for (const std::string &token : split(value, ',')) {
                const int day = std::atoi(token.c_str());
                if (day != 0 && day >= -31 && day <= 31) {
                    parsed.byMonthDays.push_back(day);
                }
            }
        } else if (key == "BYSETPOS") {
            for (const std::string &token : split(value, ',')) {
                const int pos = std::atoi(token.c_str());
                if (pos != 0 && pos >= -366 && pos <= 366) {
                    parsed.bySetPositions.push_back(pos);
                }
            }
        }
    }
    if (parsed.frequency == RecurrenceFrequency::Unknown) {
        return false;
    }
    rule = parsed;
    return true;
}

bool parseIcsDateTimeUtc(const std::string &text, int64_t &utc, bool &allDay) {
    allDay = false;
    if (text.size() == 8) {
        int year = 0;
        int month = 0;
        int day = 0;
        if (!parseDigits(text, 0, 4, year) || !parseDigits(text, 4, 2, month) ||
            !parseDigits(text, 6, 2, day)) {
            return false;
        }
        utc = recurrenceRuleDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) * 86400LL;
        allDay = true;
        return true;
    }
    if (text.size() < 15 || text[8] != 'T') {
        return false;
    }
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    if (!parseDigits(text, 0, 4, year) || !parseDigits(text, 4, 2, month) ||
        !parseDigits(text, 6, 2, day) || !parseDigits(text, 9, 2, hour) ||
        !parseDigits(text, 11, 2, minute) || !parseDigits(text, 13, 2, second)) {
        return false;
    }
    utc = recurrenceRuleDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day)) * 86400LL +
          static_cast<int64_t>(hour) * 3600LL + static_cast<int64_t>(minute) * 60LL + second;
    return true;
}

std::string formatIcsDateTimeUtc(int64_t utc, bool allDay) {
    const int64_t days = utc / 86400LL;
    int rem = static_cast<int>(utc % 86400LL);
    if (rem < 0) {
        rem += 86400;
    }
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    recurrenceRuleCivilFromDays(days, year, month, day);
    char buffer[32];
    if (allDay) {
        std::snprintf(buffer, sizeof(buffer), "%04d%02u%02u", year, month, day);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%04d%02u%02uT%02d%02d%02dZ",
                      year, month, day, rem / 3600, (rem % 3600) / 60, rem % 60);
    }
    return std::string(buffer);
}
