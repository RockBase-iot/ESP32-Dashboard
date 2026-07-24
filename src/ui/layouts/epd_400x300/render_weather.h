#pragma once

#include <stddef.h>

#include <string>
#include <vector>

#include "ui/components/calm_grid.h"

struct WeatherDayCell {
    std::string label;
    int code = 0;
    // Temperatures are stored as Celsius source values; renderers convert for display.
    int highC = 0;
    int lowC = 0;
    std::string dateLabel;
    bool today = false;
};

struct WeatherHourCell {
    std::string label;
    // Temperature is stored as a Celsius source value; renderers convert for display.
    int temperatureC = 0;
    int precipitationPct = 0;
};

struct WeatherPageSnapshot {
    std::string city;
    std::string region;
    std::string country;
    std::string updated;
    std::string currentCondition;
    std::string tempUnit;
    // Temperatures are stored as Celsius source values; renderers convert using tempUnit.
    int currentTempC = 0;
    int feelsLikeC = 0;
    int humidityPct = 0;
    int windKph = 0;
    int rainPct = 0;
    int pressureHpa = 0;
    int visibilityKm = 0;
    int indoorTempC = 0;
    int indoorHumidityPct = 0;
    std::vector<WeatherDayCell> weekly;
    std::vector<WeatherHourCell> hourly;
};

WeatherPageSnapshot sampleWeatherPageSnapshot();
void renderWeatherTodayPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot,
                            size_t pageNumber = 6, size_t pageCount = 8,
                            const std::string &ipText = "IP: --",
                            calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderWeeklyWeatherPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot,
                             size_t pageNumber = 6, size_t pageCount = 8,
                             const std::string &ipText = "IP: --",
                             calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderIndoorClimatePage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot,
                             size_t pageNumber = 6, size_t pageCount = 8,
                             const std::string &ipText = "IP: --",
                             calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
