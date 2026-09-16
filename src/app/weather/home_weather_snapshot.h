#pragma once

#include <string>
#include <vector>

#include "app/config/app_config.h"
#include "app/weather/weather_data.h"

struct HomeWeatherHour {
    std::string timeLabel;
    float temperatureC = NAN;
    float precipitationMm = NAN;
};

struct HomeWeatherSnapshot {
    bool valid = false;
    std::string location;
    std::string condition;
    std::string temperatureUnit = "C";
    std::string windUnit = "m/s";
    float currentTempC = NAN;
    float lowTempC = NAN;
    float highTempC = NAN;
    float cloudCoverPct = NAN;
    float currentPrecipitationMm = NAN;
    float windMps = NAN;
    float windDisplayValue = NAN;
    bool isDay = true;
    std::vector<HomeWeatherHour> hourly;
};

HomeWeatherSnapshot buildHomeWeatherSnapshot(const WeatherData &weather,
                                             const AppConfig &config);
bool homeCurrentRainActive(const HomeWeatherSnapshot &snapshot);
bool homeHourlyRainActive(const HomeWeatherHour &hour);
std::string homeRainOutlook(const HomeWeatherSnapshot &snapshot);
