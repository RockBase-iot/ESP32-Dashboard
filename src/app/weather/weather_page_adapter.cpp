#include "weather_page_adapter.h"

#if defined(UI_LAYOUT_EPD_400x300)
#include <cstdio>
#include <ctime>

namespace {
const char *weekdayLabel(int wday) {
    static constexpr const char *kLabels[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    return wday >= 0 && wday <= 6 ? kLabels[wday] : "MON";
}

const char *monthLabel(int month) {
    static constexpr const char *kLabels[] = {
        "", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
    };
    return month >= 1 && month <= 12 ? kLabels[month] : "JAN";
}

tm utcParts(int64_t epoch) {
    const time_t value = static_cast<time_t>(epoch);
    tm parts = {};
#if defined(_WIN32)
    gmtime_s(&parts, &value);
#else
    gmtime_r(&value, &parts);
#endif
    return parts;
}

std::string dateLabel(const tm &parts) {
    char buffer[12] = {};
    std::snprintf(buffer, sizeof(buffer), "%s %02d", monthLabel(parts.tm_mon + 1), parts.tm_mday);
    return buffer;
}

std::string normalizedTempUnit(const std::string &unit) {
    return unit == "F" || unit == "f" ? "F" : "C";
}
}  // namespace

WeatherPageSnapshot weatherFallbackPageSnapshot(const std::string &city,
                                                const std::string &region,
                                                const std::string &country,
                                                const std::string &tempUnit,
                                                int64_t localEpoch) {
    WeatherPageSnapshot snapshot;
    snapshot.city = city.empty() ? "WEATHER" : city;
    snapshot.region = region;
    snapshot.country = country;
    snapshot.updated = "Weather setup";
    snapshot.currentCondition = "No live data";
    snapshot.tempUnit = normalizedTempUnit(tempUnit);
    snapshot.currentTempC = 22;
    snapshot.feelsLikeC = 22;
    snapshot.humidityPct = 0;
    snapshot.windKph = 0;
    snapshot.rainPct = 0;
    snapshot.pressureHpa = 0;
    snapshot.visibilityKm = 0;
    snapshot.indoorTempC = 22;
    snapshot.indoorHumidityPct = 0;
    snapshot.hourly = {
        {"00", 20, 0},
        {"03", 19, 0},
        {"06", 18, 0},
        {"09", 20, 0},
        {"12", 22, 0},
        {"15", 23, 0},
        {"18", 22, 0},
        {"21", 21, 0},
    };

    snapshot.weekly.reserve(7);
    for (int i = 0; i < 7; ++i) {
        const tm parts = utcParts(localEpoch + static_cast<int64_t>(i) * 86400LL);
        snapshot.weekly.push_back(WeatherDayCell{
            weekdayLabel(parts.tm_wday),
            i % 3 == 1 ? 61 : (i % 3 == 2 ? 2 : 1),
            22 + (i % 3),
            15 + (i % 2),
            dateLabel(parts),
            i == 0,
        });
    }
    return snapshot;
}
#endif
