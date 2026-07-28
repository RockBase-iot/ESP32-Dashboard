#include "timezone_resolver.h"

#include <algorithm>
#include <cctype>

#include "app/time/timezone_catalog.h"

namespace {
int64_t timezoneResolverDaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

int dayOfWeek(int year, int month, int day) {
    const int64_t epochDays = timezoneResolverDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    const int value = static_cast<int>((epochDays + 4) % 7);
    return value < 0 ? value + 7 : value;
}

int nthSunday(int year, int month, int ordinal) {
    const int firstDow = dayOfWeek(year, month, 1);
    const int firstSunday = 1 + ((7 - firstDow) % 7);
    return firstSunday + (ordinal - 1) * 7;
}

int lastSunday(int year, int month) {
    static const int monthDays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int days = monthDays[month];
    const bool leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    if (month == 2 && leap) {
        ++days;
    }
    const int dow = dayOfWeek(year, month, days);
    return days - dow;
}

int64_t localEpoch(const LocalDateTime &local) {
    return timezoneResolverDaysFromCivil(local.year, static_cast<unsigned>(local.month), static_cast<unsigned>(local.day)) *
               86400LL +
           static_cast<int64_t>(local.hour) * 3600LL +
           static_cast<int64_t>(local.minute) * 60LL +
           local.second;
}

bool isUtcZone(const std::string &timezoneId) {
    const std::string canonical = canonicalTimezoneId(timezoneId);
    return canonical == "Etc/UTC" || canonical == "UTC" || canonical == "Etc/GMT" ||
           canonical == "GMT";
}

int offsetForUtcZone(const std::string &timezoneId) {
    const std::string canonical = canonicalTimezoneId(timezoneId);
    for (const TimezoneCatalogEntry &entry : timezoneCatalog()) {
        if (canonical == entry.iana) {
            return entry.standardOffsetSeconds;
        }
    }
    return 0;
}

bool inNewYorkDst(const LocalDateTime &local) {
    const int startDay = nthSunday(local.year, 3, 2);
    const int endDay = nthSunday(local.year, 11, 1);
    const int64_t start = timezoneResolverDaysFromCivil(local.year, 3, static_cast<unsigned>(startDay)) * 86400LL +
                          2 * 3600LL;
    const int64_t end = timezoneResolverDaysFromCivil(local.year, 11, static_cast<unsigned>(endDay)) * 86400LL +
                        2 * 3600LL;
    const int64_t epoch = localEpoch(local);
    return epoch >= start && epoch < end;
}

bool inEuropeDst(const std::string &timezoneId, const LocalDateTime &local) {
    const int startDay = lastSunday(local.year, 3);
    const int endDay = lastSunday(local.year, 10);
    const int startHour = timezoneId == "Europe/London" ? 1 : 2;
    const int endHour = timezoneId == "Europe/London" ? 2 : 3;
    const int64_t start = timezoneResolverDaysFromCivil(local.year, 3, static_cast<unsigned>(startDay)) * 86400LL +
                          startHour * 3600LL;
    const int64_t end = timezoneResolverDaysFromCivil(local.year, 10, static_cast<unsigned>(endDay)) * 86400LL +
                        endHour * 3600LL;
    const int64_t epoch = localEpoch(local);
    return epoch >= start && epoch < end;
}

int offsetSecondsForLocal(const std::string &timezoneId, const LocalDateTime &local) {
    const std::string canonical = canonicalTimezoneId(timezoneId);
    if (isUtcZone(canonical)) {
        return 0;
    }
    if (canonical == "America/New_York" || canonical == "America/Toronto") {
        return inNewYorkDst(local) ? -4 * 3600 : -5 * 3600;
    }
    if (canonical == "America/Chicago") {
        const LocalDateTime shifted = local;
        return inNewYorkDst(shifted) ? -5 * 3600 : -6 * 3600;
    }
    if (canonical == "America/Denver") {
        const LocalDateTime shifted = local;
        return inNewYorkDst(shifted) ? -6 * 3600 : -7 * 3600;
    }
    if (canonical == "America/Los_Angeles" || canonical == "America/Vancouver") {
        const LocalDateTime shifted = local;
        return inNewYorkDst(shifted) ? -7 * 3600 : -8 * 3600;
    }
    if (canonical == "America/Anchorage") {
        const LocalDateTime shifted = local;
        return inNewYorkDst(shifted) ? -8 * 3600 : -9 * 3600;
    }
    if (canonical == "Europe/London") {
        return inEuropeDst(canonical, local) ? 3600 : 0;
    }
    if (canonical.rfind("Europe/", 0) == 0) {
        const int standard = offsetForUtcZone(canonical);
        for (const TimezoneCatalogEntry &entry : timezoneCatalog()) {
            if (canonical == entry.iana && !entry.observesDst) {
                return entry.standardOffsetSeconds;
            }
        }
        return inEuropeDst(canonical, local) ? standard + 3600 : standard;
    }
    if (canonical == "Australia/Sydney" || canonical == "Australia/Melbourne") {
        return timezoneOffsetSecondsAtUtc(canonical, localEpoch(local) - 10 * 3600);
    }
    return offsetForUtcZone(canonical);
}

int offsetSecondsForUtc(const std::string &timezoneId, int64_t utc) {
    const std::string canonical = canonicalTimezoneId(timezoneId);
    if (isUtcZone(canonical)) {
        return 0;
    }
    return timezoneOffsetSecondsAtUtc(canonical, utc);
}
}  // namespace

int64_t TimezoneResolver::toUtc(const std::string &timezoneId, const LocalDateTime &local) const {
    const int offset = offsetSecondsForLocal(timezoneId, local);
    return localEpoch(local) - offset;
}

int64_t TimezoneResolver::fromUtc(const std::string &timezoneId, int64_t utc) const {
    return utc + offsetSeconds(timezoneId, utc);
}

int TimezoneResolver::offsetSeconds(const std::string &timezoneId, int64_t utc) const {
    return offsetSecondsForUtc(timezoneId, utc);
}
