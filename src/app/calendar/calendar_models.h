#pragma once

#include <stdint.h>

#include <string>
#include <vector>

enum class CalendarEventStatus : uint8_t {
    Confirmed,
    Tentative,
    Cancelled,
    Unknown,
};

enum class CalendarEventOrigin : uint8_t {
    Remote,
    Local,
};

struct CalendarEvent {
    std::string sourceId;
    std::string uid;
    std::string recurrenceId;
    std::string summary;
    std::string location;
    std::string description;
    int64_t startUtc = 0;
    int64_t endUtc = 0;
    bool allDay = false;
    uint32_t sequence = 0;
    int64_t dtstampUtc = 0;
    CalendarEventStatus status = CalendarEventStatus::Confirmed;
    CalendarEventOrigin origin = CalendarEventOrigin::Remote;
    std::string rrule;
    std::vector<std::string> rdate;
    std::vector<std::string> exdate;
    std::string startTzid;
    std::string endTzid;
    bool startFloating = false;
    bool endFloating = false;
};

std::string truncateCalendarUtf8(const std::string &value, size_t maxBytes);
std::string unescapeIcsText(const std::string &value);
