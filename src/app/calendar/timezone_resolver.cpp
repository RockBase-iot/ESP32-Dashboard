#include "timezone_resolver.h"

#include <algorithm>
#include <cctype>

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

void timezoneResolverCivilFromDays(int64_t z, int &year, unsigned &month, unsigned &day) {
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

LocalDateTime utcToLocalDateTime(int64_t utc, int offsetSeconds) {
    const int64_t local = utc + offsetSeconds;
    const int64_t days = local / 86400LL;
    int rem = static_cast<int>(local % 86400LL);
    if (rem < 0) {
        rem += 86400;
    }
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    timezoneResolverCivilFromDays(days, year, month, day);
    LocalDateTime out;
    out.year = year;
    out.month = static_cast<int>(month);
    out.day = static_cast<int>(day);
    out.hour = rem / 3600;
    out.minute = (rem % 3600) / 60;
    out.second = rem % 60;
    return out;
}

bool isUtcZone(const std::string &timezoneId) {
    return timezoneId == "Etc/UTC" || timezoneId == "UTC" || timezoneId == "Etc/GMT" ||
           timezoneId == "GMT";
}

int offsetForUtcZone(const std::string &timezoneId) {
    if (timezoneId == "Asia/Shanghai") return 8 * 3600;
    if (timezoneId == "Asia/Tokyo") return 9 * 3600;
    if (timezoneId == "Europe/Berlin") return 3600;
    if (timezoneId == "Europe/London") return 0;
    if (timezoneId == "America/Denver") return -7 * 3600;
    if (timezoneId == "America/Chicago") return -6 * 3600;
    if (timezoneId == "America/New_York") return -5 * 3600;
    if (timezoneId == "America/Los_Angeles") return -8 * 3600;
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
    if (isUtcZone(timezoneId)) {
        return 0;
    }
    if (timezoneId == "America/New_York") {
        return inNewYorkDst(local) ? -4 * 3600 : -5 * 3600;
    }
    if (timezoneId == "America/Chicago") {
        const LocalDateTime shifted = local;
        return inNewYorkDst(shifted) ? -5 * 3600 : -6 * 3600;
    }
    if (timezoneId == "America/Denver") {
        const LocalDateTime shifted = local;
        return inNewYorkDst(shifted) ? -6 * 3600 : -7 * 3600;
    }
    if (timezoneId == "America/Los_Angeles") {
        const LocalDateTime shifted = local;
        return inNewYorkDst(shifted) ? -7 * 3600 : -8 * 3600;
    }
    if (timezoneId == "Europe/London") {
        return inEuropeDst(timezoneId, local) ? 3600 : 0;
    }
    if (timezoneId == "Europe/Berlin") {
        return inEuropeDst(timezoneId, local) ? 2 * 3600 : 3600;
    }
    if (timezoneId == "Asia/Shanghai") {
        return 8 * 3600;
    }
    if (timezoneId == "Asia/Tokyo") {
        return 9 * 3600;
    }
    return 0;
}

int offsetSecondsForUtc(const std::string &timezoneId, int64_t utc) {
    if (isUtcZone(timezoneId)) {
        return 0;
    }
    if (timezoneId == "America/New_York") {
        const LocalDateTime localStd = utcToLocalDateTime(utc, -5 * 3600);
        return inNewYorkDst(localStd) ? -4 * 3600 : -5 * 3600;
    }
    if (timezoneId == "Europe/London") {
        const LocalDateTime localStd = utcToLocalDateTime(utc, 0);
        return inEuropeDst(timezoneId, localStd) ? 3600 : 0;
    }
    if (timezoneId == "Europe/Berlin") {
        const LocalDateTime localStd = utcToLocalDateTime(utc, 3600);
        return inEuropeDst(timezoneId, localStd) ? 2 * 3600 : 3600;
    }
    return offsetForUtcZone(timezoneId);
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
