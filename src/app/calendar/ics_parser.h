#pragma once

#include <stdint.h>

#include "app/calendar/calendar_models.h"
#include "app/calendar/ics_line_reader.h"
#include "app/model/dashboard_models.h"

struct IcsParserOptions {
    size_t maxSummaryBytes = 160;
    size_t maxLocationBytes = 160;
    size_t maxDescriptionBytes = 1024;
    std::string defaultTimezoneId = "Etc/UTC";
};

enum class IcsParseError : uint8_t {
    None = 0,
    InvalidLine,
    MissingCalendarBegin,
    MissingCalendarEnd,
    TruncatedEvent,
};

const char *icsParseErrorName(IcsParseError error);

struct IcsParseResult {
    SourceState state = SourceState::Ok;
    IcsParseError error = IcsParseError::None;
    uint32_t eventCount = 0;
    uint32_t skippedCount = 0;
    uint32_t byteCount = 0;
    uint32_t lineCount = 0;
};

class IcsEventSink {
public:
    virtual ~IcsEventSink() = default;
    virtual bool onEvent(const CalendarEvent &event) = 0;
};

class IcsParser {
public:
    IcsParseResult parse(const std::string &sourceId, IcsByteReader &reader,
                         IcsEventSink &sink,
                         const IcsParserOptions &options = IcsParserOptions()) const;
};
