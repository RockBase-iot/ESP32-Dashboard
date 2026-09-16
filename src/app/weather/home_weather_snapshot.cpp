#include "home_weather_snapshot.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
constexpr float kHourlyRainThresholdMm = 0.05f;

std::string trimUpperCity(const String &configuredCity) {
    std::string value = configuredCity.c_str();
    const size_t comma = value.find(',');
    if (comma != std::string::npos) {
        value.resize(comma);
    }
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    value = value.substr(begin, end - begin);
    for (char &character : value) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }
    return value.empty() ? "WEATHER" : value;
}

std::string upperCondition(int code, bool isDay) {
    if (code == 0) return isDay ? "SUNNY" : "CLEAR";
    if (code == 1) return isDay ? "MOSTLY SUNNY" : "MOSTLY CLEAR";
    if (code == 2) return "PARTLY CLOUDY";
    if (code == 3) return "CLOUDY";
    if (code == 45 || code == 48) return "FOG";
    if (code >= 51 && code <= 57) return "DRIZZLE";
    if (code >= 61 && code <= 67) return "RAIN";
    if (code >= 71 && code <= 77) return "SNOW";
    if (code >= 80 && code <= 82) return "SHOWERS";
    if (code >= 95) return "STORM";
    return "WEATHER";
}

bool validIsoHour(const String &timestamp) {
    const int separator = timestamp.indexOf('T');
    return separator >= 0 && timestamp.length() >= static_cast<size_t>(separator + 3) &&
           std::isdigit(static_cast<unsigned char>(timestamp[separator + 1])) &&
           std::isdigit(static_cast<unsigned char>(timestamp[separator + 2]));
}

std::string hourLabel(const String &timestamp) {
    const int separator = timestamp.indexOf('T');
    return timestamp.substring(separator + 1, separator + 3).c_str();
}

void setWindDisplay(HomeWeatherSnapshot &snapshot, const AppConfig &config,
                    float sourceWindKph) {
    snapshot.windMps = std::isfinite(sourceWindKph) ? sourceWindKph / 3.6f : NAN;
    if (config.unitsSpeed == "mph") {
        snapshot.windDisplayValue = sourceWindKph * 0.621371f;
        snapshot.windUnit = "mph";
    } else if (config.unitsSpeed == "kmh") {
        snapshot.windDisplayValue = sourceWindKph;
        snapshot.windUnit = "km/h";
    } else if (config.unitsSpeed == "kn") {
        snapshot.windDisplayValue = sourceWindKph * 0.539957f;
        snapshot.windUnit = "kn";
    } else {
        snapshot.windDisplayValue = snapshot.windMps;
        snapshot.windUnit = "m/s";
    }
}
}  // namespace

HomeWeatherSnapshot buildHomeWeatherSnapshot(const WeatherData &weather,
                                             const AppConfig &config) {
    HomeWeatherSnapshot snapshot;
    snapshot.location = trimUpperCity(config.city);
    snapshot.temperatureUnit = config.unitsTemp == "F" ? "F" : "C";
    snapshot.valid = weather.valid && std::isfinite(weather.current.temperature);
    snapshot.condition = snapshot.valid
                             ? upperCondition(weather.current.weather_code, weather.current.is_day)
                             : "WEATHER UNAVAILABLE";
    snapshot.currentTempC = weather.current.temperature;
    snapshot.cloudCoverPct = std::isfinite(weather.current.cloud_cover)
                                 ? std::clamp(weather.current.cloud_cover, 0.0f, 100.0f)
                                 : NAN;
    snapshot.currentPrecipitationMm = std::isfinite(weather.current.precipitation)
                                          ? std::max(0.0f, weather.current.precipitation)
                                          : NAN;
    snapshot.isDay = weather.current.is_day;
    setWindDisplay(snapshot, config, weather.current.wind_speed);

    if (!weather.daily.empty()) {
        snapshot.lowTempC = weather.daily.front().temp_min;
        snapshot.highTempC = weather.daily.front().temp_max;
    }

    snapshot.hourly.reserve(std::min<size_t>(12, weather.hourly.size()));
    for (const WeatherHourly &hour : weather.hourly) {
        if (snapshot.hourly.size() >= 12) {
            break;
        }
        if (!validIsoHour(hour.time)) {
            continue;
        }
        if (weather.current.time.length() > 0 && hour.time < weather.current.time) {
            continue;
        }
        HomeWeatherHour point;
        point.timeLabel = hourLabel(hour.time);
        point.temperatureC = std::isfinite(hour.temperature) ? hour.temperature : NAN;
        point.precipitationMm = std::isfinite(hour.precipitation)
                                    ? std::max(0.0f, hour.precipitation)
                                    : NAN;
        snapshot.hourly.push_back(point);
    }
    return snapshot;
}

bool homeCurrentRainActive(const HomeWeatherSnapshot &snapshot) {
    return std::isfinite(snapshot.currentPrecipitationMm) &&
           snapshot.currentPrecipitationMm > 0.0f;
}

bool homeHourlyRainActive(const HomeWeatherHour &hour) {
    return std::isfinite(hour.precipitationMm) &&
           hour.precipitationMm > kHourlyRainThresholdMm;
}

std::string homeRainOutlook(const HomeWeatherSnapshot &snapshot) {
    bool hasPrecipitationData = false;
    for (const HomeWeatherHour &hour : snapshot.hourly) {
        if (std::isfinite(hour.precipitationMm)) {
            hasPrecipitationData = true;
        }
        if (homeHourlyRainActive(hour)) {
            return "RAIN FROM " + hour.timeLabel + ":00";
        }
    }
    return hasPrecipitationData ? "DRY NEXT 24H" : "RAIN DATA --";
}
