#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>

static constexpr size_t CALENDAR_SOURCE_MAX_COUNT = 20;
static constexpr size_t CALENDAR_SOURCE_MAX_URL_LENGTH = 2048;
static constexpr uint8_t CALENDAR_REDIRECT_MAX_HOPS = 3;

enum class CalendarUrlPolicyStatus : uint8_t {
    Ok,
    RejectedEmpty,
    RejectedScheme,
    RejectedUserInfo,
    RejectedTooLong,
    RejectedMissingHost,
    RejectedRedirectLimit,
};

struct CalendarUrlPolicyResult {
    bool ok = false;
    CalendarUrlPolicyStatus status = CalendarUrlPolicyStatus::RejectedEmpty;
    std::string normalizedUrl;
    std::string host;
};

struct CalendarSourceSecrets {
    uint8_t index = 0;
    std::string url;
    std::string apiKey;
    std::string alias;
    bool enabled = false;
    uint8_t color = 0;
};

struct CalendarSourceMetadata {
    uint8_t index = 0;
    bool configured = false;
    bool enabled = false;
    uint8_t color = 0;
    std::string alias;
    std::string host;
    std::string maskedUrl;
    std::string maskedApiKey;
};

CalendarUrlPolicyResult normalizeCalendarSourceUrl(const std::string &rawUrl);
CalendarUrlPolicyResult validateCalendarRedirectTarget(const std::string &fromUrl,
                                                        const std::string &toUrl,
                                                        uint8_t redirectHop);
bool isValidCalendarSourceIndex(uint8_t index);
std::string maskCalendarSecret(const std::string &value);
std::string maskCalendarUrlForDisplay(const std::string &normalizedUrl,
                                      const std::string &host);
