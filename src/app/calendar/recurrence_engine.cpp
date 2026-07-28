#include "recurrence_engine.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>

#include "app/calendar/timezone_resolver.h"

namespace {
int dayOfWeek(int64_t utc) {
    const int64_t days = utc / 86400LL;
    int dow = static_cast<int>((days + 4) % 7);
    return dow < 0 ? dow + 7 : dow;
}

int64_t recurrenceEngineDaysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? static_cast<unsigned>(-3) : 9)) + 2) / 5 +
                         day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468LL;
}

void recurrenceEngineCivilFromDays(int64_t z, int &year, unsigned &month, unsigned &day) {
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

bool recurrenceEngineIsLeap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int recurrenceEngineDaysInMonth(int year, int month) {
    static const int monthDays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && recurrenceEngineIsLeap(year)) {
        return 29;
    }
    if (month < 1 || month > 12) {
        return 31;
    }
    return monthDays[month];
}

int recurrenceEngineWeekdayForDate(int year, int month, int day) {
    const int64_t days =
        recurrenceEngineDaysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    int dow = static_cast<int>((days + 4) % 7);
    return dow < 0 ? dow + 7 : dow;
}

LocalDateTime localDateTimeFromEpoch(int64_t epochSeconds) {
    const int64_t days = epochSeconds / 86400LL;
    int rem = static_cast<int>(epochSeconds % 86400LL);
    if (rem < 0) {
        rem += 86400;
    }
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
    recurrenceEngineCivilFromDays(days, year, month, day);
    return LocalDateTime{year, static_cast<int>(month), static_cast<int>(day),
                         rem / 3600, (rem % 3600) / 60, rem % 60};
}

std::string timezoneForEvent(const CalendarEvent &event, const std::string &defaultTimezoneId) {
    if (!event.startTzid.empty()) {
        return event.startTzid;
    }
    if (event.startFloating && !defaultTimezoneId.empty()) {
        return defaultTimezoneId;
    }
    return "Etc/UTC";
}

LocalDateTime localStartForEvent(const CalendarEvent &event,
                                 const std::string &defaultTimezoneId,
                                 const TimezoneResolver &resolver) {
    const std::string zone = timezoneForEvent(event, defaultTimezoneId);
    return localDateTimeFromEpoch(resolver.fromUtc(zone, event.startUtc));
}

void addMonths(int startYear, int startMonth, int monthOffset, int &year, int &month) {
    const int zeroBased = (startMonth - 1) + monthOffset;
    year = startYear + zeroBased / 12;
    month = zeroBased % 12 + 1;
}

void appendUniqueDay(std::vector<int> &days, int day) {
    if (std::find(days.begin(), days.end(), day) == days.end()) {
        days.push_back(day);
    }
}

std::vector<int> monthlyCandidateDays(const RecurrenceRule &rule,
                                      const LocalDateTime &masterLocal,
                                      int year, int month) {
    const int daysInMonth = recurrenceEngineDaysInMonth(year, month);
    std::vector<int> days;

    if (!rule.byWeekdays.empty()) {
        for (int day = 1; day <= daysInMonth; ++day) {
            const int weekday = recurrenceEngineWeekdayForDate(year, month, day);
            if (std::find(rule.byWeekdays.begin(), rule.byWeekdays.end(), weekday) !=
                rule.byWeekdays.end()) {
                days.push_back(day);
            }
        }
    } else if (!rule.byMonthDays.empty()) {
        for (int rawDay : rule.byMonthDays) {
            const int day = rawDay > 0 ? rawDay : daysInMonth + rawDay + 1;
            if (day >= 1 && day <= daysInMonth) {
                appendUniqueDay(days, day);
            }
        }
        std::sort(days.begin(), days.end());
    } else if (masterLocal.day >= 1 && masterLocal.day <= daysInMonth) {
        days.push_back(masterLocal.day);
    }

    if (!rule.bySetPositions.empty()) {
        std::vector<int> selected;
        for (int pos : rule.bySetPositions) {
            const int index = pos > 0 ? pos - 1 : static_cast<int>(days.size()) + pos;
            if (index >= 0 && index < static_cast<int>(days.size())) {
                appendUniqueDay(selected, days[static_cast<size_t>(index)]);
            }
        }
        std::sort(selected.begin(), selected.end());
        return selected;
    }
    return days;
}

int64_t occurrenceStartForDay(int64_t baseUtc, int weekdayDelta) {
    return baseUtc + static_cast<int64_t>(weekdayDelta) * 86400LL;
}

std::string canonicalOccurrenceId(int64_t utc, bool allDay) {
    return formatIcsDateTimeUtc(utc, allDay);
}

bool containsId(const std::set<std::string> &values, const std::string &id) {
    return values.find(id) != values.end();
}

std::vector<std::string> splitDateList(const std::string &text) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t comma = text.find(',', start);
        const size_t end = comma == std::string::npos ? text.size() : comma;
        if (end > start) {
            out.push_back(text.substr(start, end - start));
        }
        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }
    return out;
}

int64_t distanceFromNow(int64_t utc, int64_t nowUtc) {
    return utc >= nowUtc ? utc - nowUtc : nowUtc - utc;
}

struct ExpandedItem {
    CalendarEvent event;
    bool cancelled = false;
};

bool betterOverride(const CalendarEvent &candidate, const CalendarEvent &current) {
    if (candidate.sequence != current.sequence) {
        return candidate.sequence > current.sequence;
    }
    return candidate.dtstampUtc >= current.dtstampUtc;
}

std::vector<int> byDaysFor(const RecurrenceRule &rule, int fallbackWeekday) {
    if (!rule.byWeekdays.empty()) {
        std::vector<int> days = rule.byWeekdays;
        std::sort(days.begin(), days.end());
        days.erase(std::unique(days.begin(), days.end()), days.end());
        return days;
    }
    return {fallbackWeekday};
}

int64_t occurrenceDuration(const CalendarEvent &event) {
    if (event.endUtc > event.startUtc) {
        return event.endUtc - event.startUtc;
    }
    return event.allDay ? 86400LL : 0LL;
}

bool isWithinWindow(int64_t startUtc, const RecurrenceWindow &window) {
    if (window.endUtc > 0 && startUtc > window.endUtc) {
        return false;
    }
    if (window.startUtc > 0 && startUtc < window.startUtc) {
        return false;
    }
    return true;
}

CalendarEvent makeOccurrence(const CalendarEvent &master, int64_t startUtc, const std::string &recurrenceId) {
    CalendarEvent occurrence = master;
    occurrence.startUtc = startUtc;
    const int64_t duration = occurrenceDuration(master);
    if (duration > 0) {
        occurrence.endUtc = startUtc + duration;
    }
    occurrence.recurrenceId = recurrenceId;
    return occurrence;
}

void appendExplicitOccurrence(std::map<std::string, CalendarEvent> &occurrences, const CalendarEvent &event) {
    const std::string key = event.recurrenceId.empty() ? canonicalOccurrenceId(event.startUtc, event.allDay)
                                                       : event.recurrenceId;
    const auto it = occurrences.find(key);
    if (it == occurrences.end() || betterOverride(event, it->second)) {
        occurrences[key] = event;
    }
}
}  // namespace

std::vector<CalendarEvent> RecurrenceEngine::expand(const std::vector<CalendarEvent> &events,
                                                    const RecurrenceWindow &window,
                                                    const std::string &defaultTimezoneId,
                                                    SourceState &state) const {
    std::vector<CalendarEvent> result;
    state = SourceState::Ok;

    std::map<std::string, CalendarEvent> overrides;
    std::set<std::string> exclusions;
    std::vector<CalendarEvent> masters;
    std::vector<CalendarEvent> singles;

    for (const CalendarEvent &event : events) {
        if (!event.recurrenceId.empty()) {
            const std::string key = event.recurrenceId;
            if (event.status == CalendarEventStatus::Cancelled) {
                exclusions.insert(key);
            } else {
                appendExplicitOccurrence(overrides, event);
            }
            continue;
        }
        if (!event.rrule.empty() || !event.rdate.empty()) {
            masters.push_back(event);
        } else {
            singles.push_back(event);
        }
    }

    std::map<std::string, CalendarEvent> occurrences;
    for (const CalendarEvent &single : singles) {
        const std::string key = canonicalOccurrenceId(single.startUtc, single.allDay);
        if (!isWithinWindow(single.startUtc, window)) {
            continue;
        }
        if (!containsId(exclusions, key)) {
            occurrences[key] = single;
        }
    }

    for (const CalendarEvent &master : masters) {
        const int64_t duration = occurrenceDuration(master);
        RecurrenceRule rule;
        if (!master.rrule.empty() && !parseRecurrenceRule(master.rrule, rule)) {
            state = SourceState::Parse;
            continue;
        }

        const int fallbackWeekday = dayOfWeek(master.startUtc);
        const int64_t endLimit = window.endUtc > 0 ? window.endUtc : (master.startUtc + 365LL * 86400LL);

        std::set<std::string> masterExclusions = exclusions;
        for (const std::string &raw : master.exdate) {
            for (const std::string &token : splitDateList(raw)) {
                int64_t utc = 0;
                bool allDay = false;
                if (parseIcsDateTimeUtc(token, utc, allDay)) {
                    masterExclusions.insert(canonicalOccurrenceId(utc, allDay || master.allDay));
                }
            }
        }

        if (master.rrule.empty()) {
            const std::string key = canonicalOccurrenceId(master.startUtc, master.allDay);
            if (isWithinWindow(master.startUtc, window) && !containsId(masterExclusions, key)) {
                occurrences[key] = master;
            }
        } else if (rule.frequency == RecurrenceFrequency::Daily) {
            int produced = 0;
            for (int step = 0; ; ++step) {
                const int64_t startUtc = master.startUtc + static_cast<int64_t>(step) * rule.interval * 86400LL;
                if (rule.count > 0 && produced >= rule.count) {
                    break;
                }
                if (rule.untilUtc > 0 && startUtc > rule.untilUtc) {
                    break;
                }
                if (startUtc > endLimit) {
                    break;
                }
                if (!isWithinWindow(startUtc, window)) {
                    continue;
                }
                const std::string key = canonicalOccurrenceId(startUtc, master.allDay);
                if (containsId(masterExclusions, key)) {
                    continue;
                }
                occurrences[key] = makeOccurrence(master, startUtc, key);
                ++produced;
            }
        } else if (rule.frequency == RecurrenceFrequency::Weekly) {
            const std::vector<int> weekdays = byDaysFor(rule, fallbackWeekday);
            int produced = 0;
            int week = 0;
            while (true) {
                const int64_t weekBase = master.startUtc + static_cast<int64_t>(week) * rule.interval * 7LL * 86400LL;
                bool anyInWeek = false;
                for (int weekday : weekdays) {
                    const int delta = weekday - fallbackWeekday;
                    const int64_t startUtc = occurrenceStartForDay(weekBase, delta);
                    if (startUtc < master.startUtc && week == 0) {
                        continue;
                    }
                    if (rule.count > 0 && produced >= rule.count) {
                        break;
                    }
                    if (rule.untilUtc > 0 && startUtc > rule.untilUtc) {
                        continue;
                    }
                    if (startUtc > endLimit) {
                        continue;
                    }
                    if (!isWithinWindow(startUtc, window)) {
                        continue;
                    }
                    const std::string key = canonicalOccurrenceId(startUtc, master.allDay);
                    if (containsId(masterExclusions, key)) {
                        continue;
                    }
                    occurrences[key] = makeOccurrence(master, startUtc, key);
                    ++produced;
                    anyInWeek = true;
                }
                if (rule.count > 0 && produced >= rule.count) {
                    break;
                }
                if (!anyInWeek && week > 260) {
                    break;
                }
                if (weekBase > endLimit) {
                    break;
                }
                ++week;
                if (week > 520) {
                    break;
                }
            }
        } else if (rule.frequency == RecurrenceFrequency::Monthly) {
            TimezoneResolver resolver;
            const std::string zone = timezoneForEvent(master, defaultTimezoneId);
            const LocalDateTime masterLocal =
                localStartForEvent(master, defaultTimezoneId, resolver);
            int produced = 0;
            for (int monthStep = 0; monthStep <= 240; monthStep += rule.interval) {
                int year = masterLocal.year;
                int month = masterLocal.month;
                addMonths(masterLocal.year, masterLocal.month, monthStep, year, month);
                const std::vector<int> candidateDays =
                    monthlyCandidateDays(rule, masterLocal, year, month);
                bool stop = false;
                for (int day : candidateDays) {
                    if (rule.count > 0 && produced >= rule.count) {
                        stop = true;
                        break;
                    }
                    const int64_t startUtc =
                        resolver.toUtc(zone, LocalDateTime{year, month, day,
                                                           masterLocal.hour,
                                                           masterLocal.minute,
                                                           masterLocal.second});
                    if (startUtc < master.startUtc) {
                        continue;
                    }
                    if (rule.untilUtc > 0 && startUtc > rule.untilUtc) {
                        stop = true;
                        break;
                    }
                    if (startUtc > endLimit) {
                        stop = true;
                        break;
                    }
                    if (!isWithinWindow(startUtc, window)) {
                        continue;
                    }
                    const std::string key = canonicalOccurrenceId(startUtc, master.allDay);
                    if (containsId(masterExclusions, key)) {
                        continue;
                    }
                    occurrences[key] = makeOccurrence(master, startUtc, key);
                    ++produced;
                }
                if (stop || (rule.count > 0 && produced >= rule.count)) {
                    break;
                }
            }
        } else {
            const std::string key = canonicalOccurrenceId(master.startUtc, master.allDay);
            if (isWithinWindow(master.startUtc, window) && !containsId(masterExclusions, key)) {
                occurrences[key] = master;
            }
        }

        for (const std::string &raw : master.rdate) {
            for (const std::string &token : splitDateList(raw)) {
                int64_t utc = 0;
                bool allDay = false;
                if (!parseIcsDateTimeUtc(token, utc, allDay)) {
                    continue;
                }
                if (isWithinWindow(utc, window)) {
                    const std::string key = canonicalOccurrenceId(utc, allDay);
                    if (!containsId(masterExclusions, key)) {
                        CalendarEvent explicitEvent = master;
                        explicitEvent.startUtc = utc;
                        explicitEvent.endUtc = utc + (duration > 0 ? duration : (allDay ? 86400LL : 0LL));
                        explicitEvent.allDay = allDay || master.allDay;
                        explicitEvent.recurrenceId = key;
                        occurrences[key] = explicitEvent;
                    }
                }
            }
        }
    }

    for (const auto &pair : overrides) {
        if (!containsId(exclusions, pair.first) && isWithinWindow(pair.second.startUtc, window)) {
            occurrences[pair.first] = pair.second;
        }
    }

    for (const auto &pair : occurrences) {
        result.push_back(pair.second);
    }

    std::sort(result.begin(), result.end(), [](const CalendarEvent &a, const CalendarEvent &b) {
        if (a.startUtc != b.startUtc) {
            return a.startUtc < b.startUtc;
        }
        if (a.sourceId != b.sourceId) {
            return a.sourceId < b.sourceId;
        }
        return a.recurrenceId < b.recurrenceId;
    });

    if (window.capacity > 0 && result.size() > window.capacity) {
        std::sort(result.begin(), result.end(), [&](const CalendarEvent &a, const CalendarEvent &b) {
            const int64_t aDistance = distanceFromNow(a.startUtc, window.seedUtc);
            const int64_t bDistance = distanceFromNow(b.startUtc, window.seedUtc);
            if (aDistance != bDistance) {
                return aDistance < bDistance;
            }
            return a.startUtc < b.startUtc;
        });
        result.resize(window.capacity);
        state = SourceState::Limit;
        std::sort(result.begin(), result.end(), [](const CalendarEvent &a, const CalendarEvent &b) {
            if (a.startUtc != b.startUtc) {
                return a.startUtc < b.startUtc;
            }
            if (a.sourceId != b.sourceId) {
                return a.sourceId < b.sourceId;
            }
            return a.recurrenceId < b.recurrenceId;
        });
    }
    return result;
}
