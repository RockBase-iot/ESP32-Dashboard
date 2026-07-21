#pragma once

#include <stdint.h>

#include <string>
#include <vector>

enum class RecurrenceFrequency : uint8_t {
    Daily,
    Weekly,
    Monthly,
    Yearly,
    Unknown,
};

struct RecurrenceRule {
    RecurrenceFrequency frequency = RecurrenceFrequency::Unknown;
    int interval = 1;
    int count = 0;
    int64_t untilUtc = 0;
    std::vector<int> byWeekdays;
};

bool parseRecurrenceRule(const std::string &text, RecurrenceRule &rule);
bool parseIcsDateTimeUtc(const std::string &text, int64_t &utc, bool &allDay);
std::string formatIcsDateTimeUtc(int64_t utc, bool allDay = false);
