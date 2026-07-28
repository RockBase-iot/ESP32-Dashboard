#include "calendar_source.h"

#include <algorithm>
#include <cctype>

namespace {
std::string trimAscii(const std::string &text) {
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string lowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

bool startsWithInsensitive(const std::string &text, const char *prefix) {
    const std::string needle(prefix);
    if (text.size() < needle.size()) {
        return false;
    }
    return lowerAscii(text.substr(0, needle.size())) == lowerAscii(needle);
}

size_t authorityEnd(const std::string &url, size_t authorityStart) {
    const size_t slash = url.find('/', authorityStart);
    const size_t question = url.find('?', authorityStart);
    const size_t hash = url.find('#', authorityStart);
    size_t end = url.size();
    if (slash != std::string::npos) end = std::min(end, slash);
    if (question != std::string::npos) end = std::min(end, question);
    if (hash != std::string::npos) end = std::min(end, hash);
    return end;
}

std::string hostFromAuthority(std::string authority) {
    const size_t at = authority.find('@');
    if (at != std::string::npos) {
        return "";
    }
    if (!authority.empty() && authority.front() == '[') {
        const size_t close = authority.find(']');
        if (close == std::string::npos) {
            return "";
        }
        return lowerAscii(authority.substr(0, close + 1));
    }
    const size_t colon = authority.find(':');
    if (colon != std::string::npos) {
        authority = authority.substr(0, colon);
    }
    return lowerAscii(authority);
}

std::string lastFour(const std::string &value) {
    if (value.empty()) {
        return "";
    }
    if (value.size() <= 4) {
        return value;
    }
    return value.substr(value.size() - 4);
}
}  // namespace

CalendarUrlPolicyResult normalizeCalendarSourceUrl(const std::string &rawUrl) {
    CalendarUrlPolicyResult result;
    std::string url = trimAscii(rawUrl);
    if (url.empty()) {
        result.status = CalendarUrlPolicyStatus::RejectedEmpty;
        return result;
    }
    if (url.size() > CALENDAR_SOURCE_MAX_URL_LENGTH) {
        result.status = CalendarUrlPolicyStatus::RejectedTooLong;
        return result;
    }
    if (startsWithInsensitive(url, "webcal://")) {
        url = "https://" + url.substr(9);
    } else if (!startsWithInsensitive(url, "https://")) {
        result.status = CalendarUrlPolicyStatus::RejectedScheme;
        return result;
    }

    const size_t authorityStart = 8;
    const size_t end = authorityEnd(url, authorityStart);
    if (end <= authorityStart) {
        result.status = CalendarUrlPolicyStatus::RejectedMissingHost;
        return result;
    }
    const std::string authority = url.substr(authorityStart, end - authorityStart);
    if (authority.find('@') != std::string::npos) {
        result.status = CalendarUrlPolicyStatus::RejectedUserInfo;
        return result;
    }
    const std::string host = hostFromAuthority(authority);
    if (host.empty()) {
        result.status = CalendarUrlPolicyStatus::RejectedMissingHost;
        return result;
    }

    result.ok = true;
    result.status = CalendarUrlPolicyStatus::Ok;
    result.normalizedUrl = "https://" + authority + url.substr(end);
    result.host = host;
    return result;
}

CalendarUrlPolicyResult validateCalendarRedirectTarget(const std::string & /*fromUrl*/,
                                                        const std::string &toUrl,
                                                        uint8_t redirectHop) {
    if (redirectHop > CALENDAR_REDIRECT_MAX_HOPS) {
        CalendarUrlPolicyResult result;
        result.status = CalendarUrlPolicyStatus::RejectedRedirectLimit;
        return result;
    }
    return normalizeCalendarSourceUrl(toUrl);
}

bool isValidCalendarSourceIndex(uint8_t index) {
    return index < CALENDAR_SOURCE_MAX_COUNT;
}

std::string maskCalendarSecret(const std::string &value) {
    if (value.empty()) {
        return "";
    }
    return std::string("********") + lastFour(value);
}

std::string maskCalendarUrlForDisplay(const std::string &normalizedUrl,
                                      const std::string &host) {
    if (normalizedUrl.empty() || host.empty()) {
        return "";
    }
    return std::string("https://") + host + "/..." + lastFour(normalizedUrl);
}

std::string calendarSourceDiagnosticLabel(const std::string &rawUrl) {
    const auto normalized = normalizeCalendarSourceUrl(rawUrl);
    if (!normalized.ok) {
        return "(invalid calendar url)";
    }
    return maskCalendarUrlForDisplay(normalized.normalizedUrl, normalized.host);
}

uint32_t calendarSourceDiagnosticHash(const std::string &rawUrl) {
    const auto normalized = normalizeCalendarSourceUrl(rawUrl);
    if (!normalized.ok) {
        return 0;
    }
    uint32_t hash = 2166136261UL;
    for (const char c : normalized.normalizedUrl) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619UL;
    }
    return hash;
}
