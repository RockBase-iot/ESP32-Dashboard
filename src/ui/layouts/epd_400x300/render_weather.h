#pragma once

#include <string>
#include <vector>

#include "ui/canvas/draw_surface.h"

struct WeatherDayCell {
    std::string label;
    int code = 0;
    int highC = 0;
    int lowC = 0;
};

struct WeatherHourCell {
    std::string label;
    int temperatureC = 0;
    int precipitationPct = 0;
};

struct WeatherPageSnapshot {
    std::string city;
    std::string updated;
    std::string currentCondition;
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
void renderWeatherTodayPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot);
void renderWeeklyWeatherPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot);
void renderIndoorClimatePage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot);
