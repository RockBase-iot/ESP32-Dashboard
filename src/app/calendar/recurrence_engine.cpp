#include "recurrence_engine.h"

#include <algorithm>
#include <map>
#include <set>

#include "app/calendar/timezone_resolver.h"

namespace {
int dayOfWeek(int64_t utc) {
    const int64_t days = utc / 86400LL;
    int dow = static_cast<int>((days + 4) % 7);
    return dow < 0 ? dow + 7 : dow;
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
    (void)defaultTimezoneId;
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
