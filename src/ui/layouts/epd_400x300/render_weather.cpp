#include "render_weather.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#include "ui/components/calm_grid.h"

namespace {
void drawWeatherMetricCard(IDrawSurface &surface, const Rect &rect, const std::string &label,
                           const std::string &value) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 6), static_cast<int16_t>(rect.y + 12),
                     calm_grid::fitText(surface, label, static_cast<int16_t>(rect.w - 12), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 6), static_cast<int16_t>(rect.y + 27),
                     calm_grid::fitText(surface, value, static_cast<int16_t>(rect.w - 12), 1),
                     kDashboardBlack, TextAlign::Left, 1);
}

const char *weatherForecastKind(int code) {
    if (code >= 60 && code < 90) {
        return "rain";
    }
    if (code >= 2 && code < 60) {
        return "cloud";
    }
    return "sun";
}

void drawSmallSunIcon(IDrawSurface &surface, int16_t x, int16_t y, uint16_t color) {
    surface.drawRect(static_cast<int16_t>(x + 8), static_cast<int16_t>(y + 8), 8, 8, color);
    surface.drawLine(static_cast<int16_t>(x + 12), y, static_cast<int16_t>(x + 12),
                     static_cast<int16_t>(y + 5), color);
    surface.drawLine(static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 19),
                     static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 24), color);
    surface.drawLine(x, static_cast<int16_t>(y + 12), static_cast<int16_t>(x + 5),
                     static_cast<int16_t>(y + 12), color);
    surface.drawLine(static_cast<int16_t>(x + 19), static_cast<int16_t>(y + 12),
                     static_cast<int16_t>(x + 24), static_cast<int16_t>(y + 12), color);
    surface.drawLine(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 3),
                     static_cast<int16_t>(x + 6), static_cast<int16_t>(y + 6), color);
    surface.drawLine(static_cast<int16_t>(x + 18), static_cast<int16_t>(y + 18),
                     static_cast<int16_t>(x + 21), static_cast<int16_t>(y + 21), color);
    surface.drawLine(static_cast<int16_t>(x + 18), static_cast<int16_t>(y + 6),
                     static_cast<int16_t>(x + 21), static_cast<int16_t>(y + 3), color);
    surface.drawLine(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 21),
                     static_cast<int16_t>(x + 6), static_cast<int16_t>(y + 18), color);
}

void drawSmallCloudIcon(IDrawSurface &surface, int16_t x, int16_t y, uint16_t color) {
    surface.drawLine(static_cast<int16_t>(x + 3), static_cast<int16_t>(y + 15),
                     static_cast<int16_t>(x + 23), static_cast<int16_t>(y + 15), color);
    surface.drawLine(static_cast<int16_t>(x + 5), static_cast<int16_t>(y + 15),
                     static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 10), color);
    surface.drawLine(static_cast<int16_t>(x + 7), static_cast<int16_t>(y + 10),
                     static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 8), color);
    surface.drawLine(static_cast<int16_t>(x + 12), static_cast<int16_t>(y + 8),
                     static_cast<int16_t>(x + 17), static_cast<int16_t>(y + 10), color);
    surface.drawLine(static_cast<int16_t>(x + 17), static_cast<int16_t>(y + 10),
                     static_cast<int16_t>(x + 21), static_cast<int16_t>(y + 15), color);
}

void drawSmallRainIcon(IDrawSurface &surface, int16_t x, int16_t y, uint16_t color) {
    drawSmallCloudIcon(surface, x, y, color);
    for (int16_t dx : {7, 13, 19}) {
        surface.drawLine(static_cast<int16_t>(x + dx), static_cast<int16_t>(y + 20),
                         static_cast<int16_t>(x + dx - 2), static_cast<int16_t>(y + 25),
                         kDashboardBlack);
    }
}

void drawSmallWeatherIcon(IDrawSurface &surface, int16_t x, int16_t y, int code,
                          uint16_t color) {
    const std::string kind = weatherForecastKind(code);
    if (kind == "rain") {
        drawSmallRainIcon(surface, x, y, color);
    } else if (kind == "cloud") {
        drawSmallCloudIcon(surface, x, y, color);
    } else {
        drawSmallSunIcon(surface, x, y, color);
    }
}

std::string weatherDateLabel(const WeatherDayCell &day) {
    return day.dateLabel.empty() ? day.label : day.dateLabel;
}

std::vector<WeatherDayCell> weeklyWeatherWindow(const std::vector<WeatherDayCell> &weekly) {
    if (weekly.empty()) {
        return {};
    }
    const size_t count = std::min<size_t>(5, weekly.size());
    return {weekly.begin(), weekly.begin() + static_cast<std::ptrdiff_t>(count)};
}

std::vector<WeatherDayCell> weatherTodayWindow(const std::vector<WeatherDayCell> &weekly) {
    if (weekly.empty()) {
        return {};
    }
    const size_t count = std::min<size_t>(7, weekly.size());
    return {weekly.begin(), weekly.begin() + static_cast<std::ptrdiff_t>(count)};
}

std::string normalizeTempUnit(const std::string &unit) {
    return unit == "F" || unit == "f" ? "F" : "C";
}

int displayTempFromC(int tempC, const std::string &unit) {
    if (normalizeTempUnit(unit) == "F") {
        return static_cast<int>(std::lround(tempC * 9.0f / 5.0f + 32.0f));
    }
    return tempC;
}

std::string tempTextFromC(int tempC, const std::string &unit) {
    const std::string normalizedUnit = normalizeTempUnit(unit);
    return std::to_string(displayTempFromC(tempC, normalizedUnit)) + normalizedUnit;
}

std::string tempRangeTextFromC(int highC, int lowC, const std::string &unit) {
    return std::to_string(displayTempFromC(highC, unit)) + "/" +
           std::to_string(displayTempFromC(lowC, unit));
}

void drawCompactWeeklyForecast(IDrawSurface &surface, const Rect &rect,
                               const std::vector<WeatherDayCell> &days,
                               const std::string &tempUnit) {
    const size_t count = std::min<size_t>(7, days.size());
    if (count == 0) {
        return;
    }
    const int16_t cellW = static_cast<int16_t>(rect.w / static_cast<int16_t>(count));
    for (size_t i = 0; i < count; ++i) {
        const WeatherDayCell &day = days[i];
        const int16_t x = static_cast<int16_t>(rect.x + cellW * static_cast<int16_t>(i) + cellW / 2);
        const bool today = day.label == "WED";
        surface.drawText(x, static_cast<int16_t>(rect.y + 4), day.label,
                         today ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
        surface.drawText(x, static_cast<int16_t>(rect.y + 16),
                         tempRangeTextFromC(day.highC, day.lowC, tempUnit),
                         today ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
    }
}

void drawWeatherTodayHourlyChart(IDrawSurface &surface, const Rect &rect,
                                 const std::vector<WeatherHourCell> &hours,
                                 const std::string &tempUnit) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 12), static_cast<int16_t>(rect.y + 10),
                     "HOURLY", kDashboardAccent, TextAlign::Left, 1);
    const int16_t chartLeft = static_cast<int16_t>(rect.x + 30);
    const int16_t chartRight = static_cast<int16_t>(rect.x + rect.w - 20);
    const int16_t chartTop = static_cast<int16_t>(rect.y + 24);
    const int16_t chartBottom = static_cast<int16_t>(rect.y + rect.h - 20);
    const size_t count = std::min<size_t>(8, hours.size());
    if (count == 0) {
        return;
    }

    int minTemp = displayTempFromC(hours.front().temperatureC, tempUnit);
    int maxTemp = minTemp;
    for (size_t i = 1; i < count; ++i) {
        const int displayTemp = displayTempFromC(hours[i].temperatureC, tempUnit);
        minTemp = std::min(minTemp, displayTemp);
        maxTemp = std::max(maxTemp, displayTemp);
    }
    int axisMin = static_cast<int>(std::floor(minTemp / 5.0f) * 5) - 5;
    int axisMax = static_cast<int>(std::ceil(maxTemp / 5.0f) * 5) + 5;
    if (axisMax - axisMin < 10) {
        axisMin -= 5;
        axisMax += 5;
    }
    const int axisRange = std::max(1, axisMax - axisMin);
    const int tickStep = axisRange > 25 ? 10 : 5;
    for (int temp = axisMax; temp >= axisMin; temp -= tickStep) {
        const int16_t y = static_cast<int16_t>(
            chartBottom - (temp - axisMin) * (chartBottom - chartTop) / axisRange);
        surface.drawLine(chartLeft, y, chartRight, y,
                         temp == axisMax ? kDashboardBlack : kDashboardWhite);
        surface.drawText(static_cast<int16_t>(chartLeft - 8), y, std::to_string(temp),
                         kDashboardBlack, TextAlign::Right, 1);
    }

    const int16_t step = count <= 1 ? 0 : static_cast<int16_t>((chartRight - chartLeft) /
                                                               static_cast<int16_t>(count - 1));
    auto tempY = [chartTop, chartBottom, axisMin, axisMax, axisRange](int temp) {
        const int clamped = std::max(axisMin, std::min(axisMax, temp));
        return static_cast<int16_t>(
            chartBottom - (clamped - axisMin) * (chartBottom - chartTop) / axisRange);
    };
    for (size_t i = 0; i < count; ++i) {
        const int16_t x = static_cast<int16_t>(chartLeft + static_cast<int16_t>(i) * step);
        if (hours[i].precipitationPct > 0) {
            const int16_t barH = static_cast<int16_t>(
                std::min(44, std::max(4, hours[i].precipitationPct / 2)));
            surface.drawRect(static_cast<int16_t>(x - 4), static_cast<int16_t>(chartBottom - barH),
                             8, barH, kDashboardBlack);
        }
    }
    for (size_t i = 0; i < count; ++i) {
        const int16_t x = static_cast<int16_t>(chartLeft + static_cast<int16_t>(i) * step);
        const int16_t y = tempY(displayTempFromC(hours[i].temperatureC, tempUnit));
        if (i > 0) {
            const int16_t px = static_cast<int16_t>(chartLeft + static_cast<int16_t>(i - 1) * step);
            const int16_t py = tempY(displayTempFromC(hours[i - 1].temperatureC, tempUnit));
            surface.drawLine(px, py, x, y, kDashboardAccent);
            surface.drawLine(px, static_cast<int16_t>(py - 1), x, static_cast<int16_t>(y - 1),
                             kDashboardAccent);
        }
        surface.fillRect(static_cast<int16_t>(x - 1), static_cast<int16_t>(y - 1), 3, 3,
                         kDashboardAccent);
        surface.drawText(x, static_cast<int16_t>(rect.y + rect.h - 10), hours[i].label,
                         kDashboardBlack, TextAlign::Center, 1);
    }
}

void drawWeeklyWeatherTrend(IDrawSurface &surface, const Rect &rect,
                            const std::vector<WeatherDayCell> &days,
                            const std::string &tempUnit) {
    if (days.empty()) {
        return;
    }
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 12), static_cast<int16_t>(rect.y + 15),
                     "5-DAY TREND", kDashboardAccent, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + rect.w - 12),
                     static_cast<int16_t>(rect.y + 15), "HIGH / LOW",
                     kDashboardBlack, TextAlign::Right, 1);

    int minTemp = displayTempFromC(days.front().lowC, tempUnit);
    int maxTemp = displayTempFromC(days.front().highC, tempUnit);
    for (const auto &day : days) {
        minTemp = std::min(minTemp, displayTempFromC(day.lowC, tempUnit));
        maxTemp = std::max(maxTemp, displayTempFromC(day.highC, tempUnit));
    }
    minTemp = static_cast<int>(std::floor(minTemp / 5.0f) * 5) - 5;
    maxTemp = static_cast<int>(std::ceil(maxTemp / 5.0f) * 5) + 5;
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 10;
    }

    const int16_t chartLeft = static_cast<int16_t>(rect.x + 42);
    const int16_t chartRight = static_cast<int16_t>(rect.x + rect.w - 34);
    const int16_t chartTop = static_cast<int16_t>(rect.y + 36);
    const int16_t chartBottom = static_cast<int16_t>(rect.y + rect.h - 28);
    const auto tempY = [chartTop, chartBottom, minTemp, maxTemp](int value) {
        const int range = maxTemp - minTemp;
        return static_cast<int16_t>(chartBottom -
                                    (value - minTemp) * (chartBottom - chartTop) / range);
    };
    const int tickStep = maxTemp - minTemp > 25 ? 10 : 5;
    for (int tick = minTemp; tick <= maxTemp; tick += tickStep) {
        const int16_t y = tempY(tick);
        surface.drawLine(chartLeft, y, chartRight, y,
                         tick == minTemp ? kDashboardBlack : kDashboardWhite);
        surface.drawText(static_cast<int16_t>(chartLeft - 10), y, std::to_string(tick),
                         kDashboardBlack, TextAlign::Right, 1);
    }

    const int16_t step = days.size() <= 1
        ? 0
        : static_cast<int16_t>((chartRight - chartLeft) / static_cast<int16_t>(days.size() - 1));
    for (size_t i = 0; i < days.size(); ++i) {
        const int16_t x = static_cast<int16_t>(chartLeft + step * static_cast<int16_t>(i));
        const int16_t highY = tempY(displayTempFromC(days[i].highC, tempUnit));
        const int16_t lowY = tempY(displayTempFromC(days[i].lowC, tempUnit));
        if (i > 0) {
            const int16_t px = static_cast<int16_t>(chartLeft + step * static_cast<int16_t>(i - 1));
            surface.drawLine(px, tempY(displayTempFromC(days[i - 1].highC, tempUnit)), x, highY,
                             kDashboardAccent);
            surface.drawLine(px, tempY(displayTempFromC(days[i - 1].lowC, tempUnit)), x, lowY,
                             kDashboardBlack);
        }
        surface.fillRect(static_cast<int16_t>(x - 2), static_cast<int16_t>(highY - 2), 5, 5,
                         kDashboardAccent);
        surface.fillRect(static_cast<int16_t>(x - 1), static_cast<int16_t>(lowY - 1), 3, 3,
                         kDashboardBlack);
        surface.drawText(x, static_cast<int16_t>(rect.y + rect.h - 10), days[i].label,
                         days[i].label == "WED" ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Center, 1);
    }
}

void drawWeeklyWeatherSummary(IDrawSurface &surface, const Rect &rect,
                              const std::vector<WeatherDayCell> &days,
                              const std::string &tempUnit) {
    if (days.empty()) {
        return;
    }
    const int16_t gap = 4;
    const int16_t cellW = static_cast<int16_t>((rect.w - gap * static_cast<int16_t>(days.size() - 1)) /
                                               static_cast<int16_t>(days.size()));
    for (size_t i = 0; i < days.size(); ++i) {
        const WeatherDayCell &day = days[i];
        const int16_t x = static_cast<int16_t>(rect.x + static_cast<int16_t>(i) * (cellW + gap));
        const bool today = day.label == "WED";
        const std::string caption = i == 0 ? "YESTERDAY" : (today ? "TODAY" : day.label);
        const Rect cell{x, rect.y, cellW, rect.h};
        surface.drawRect(cell.x, cell.y, cell.w, cell.h, today ? kDashboardAccent : kDashboardBlack);
        surface.drawText(static_cast<int16_t>(cell.x + cell.w / 2),
                         static_cast<int16_t>(cell.y + 13), caption,
                         today ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
        surface.drawText(static_cast<int16_t>(cell.x + cell.w / 2),
                         static_cast<int16_t>(cell.y + 26), weatherDateLabel(day),
                         kDashboardBlack, TextAlign::Center, 1);
        drawSmallWeatherIcon(surface, static_cast<int16_t>(cell.x + cell.w / 2 - 12),
                             static_cast<int16_t>(cell.y + 38), day.code,
                             today ? kDashboardAccent : kDashboardBlack);
        surface.drawText(static_cast<int16_t>(cell.x + cell.w / 2),
                         static_cast<int16_t>(cell.y + cell.h - 12),
                         tempRangeTextFromC(day.highC, day.lowC, tempUnit),
                         today ? kDashboardAccent : kDashboardBlack, TextAlign::Center, 1);
    }
}
}  // namespace

WeatherPageSnapshot sampleWeatherPageSnapshot() {
    WeatherPageSnapshot snapshot;
    snapshot.city = "NEW YORK";
    snapshot.region = "NY";
    snapshot.country = "USA";
    snapshot.updated = "Updated 08:30";
    snapshot.currentCondition = "Sunny";
    snapshot.tempUnit = "F";
    snapshot.currentTempC = 22;
    snapshot.feelsLikeC = 22;
    snapshot.humidityPct = 46;
    snapshot.windKph = 4;
    snapshot.rainPct = 10;
    snapshot.indoorTempC = 23;
    snapshot.indoorHumidityPct = 46;
    snapshot.weekly = {
        {"TUE", 3, 21, 15, "JUL 21"},
        {"WED", 61, 20, 14, "JUL 22"},
        {"THU", 1, 24, 16, "JUL 23"},
        {"FRI", 1, 24, 17, "JUL 24"},
        {"SAT", 3, 22, 14, "JUL 25"},
        {"SUN", 1, 23, 15, "JUL 26"},
        {"MON", 1, 23, 15, "JUL 27"},
        {"TUE", 2, 24, 16, "JUL 28"},
    };
    snapshot.hourly = {
        {"00", 20, 8},
        {"03", 18, 32},
        {"06", 16, 70},
        {"09", 18, 38},
        {"12", 21, 0},
        {"15", 22, 0},
        {"18", 22, 0},
        {"21", 21, 10},
    };
    return snapshot;
}

void renderWeatherTodayPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot,
                            size_t pageNumber, size_t pageCount) {
    calm_grid::drawPrototypePageChrome(surface, "WEATHER TODAY",
                                       calm_grid::PageIconKind::Weather, pageNumber, pageCount);

    surface.drawRect(18, 58, 118, 166, kDashboardBlack);
    surface.drawText(30, 65, calm_grid::fitText(surface, snapshot.city, 92, 1),
                     kDashboardBlack, TextAlign::Left, 2);
    if (!snapshot.region.empty()) {
        surface.drawText(30, 101, calm_grid::fitText(surface, snapshot.region, 92, 1),
                         kDashboardBlack, TextAlign::Left, 1);
    }
    if (!snapshot.country.empty()) {
        surface.drawText(30, 120, calm_grid::fitText(surface, snapshot.country, 92, 1),
                         kDashboardBlack, TextAlign::Left, 1);
    }
    surface.drawLine(30, 132, 126, 132, kDashboardBlack);
    const std::string tempUnit = normalizeTempUnit(snapshot.tempUnit);
    surface.drawText(30, 162, tempTextFromC(snapshot.currentTempC, tempUnit),
                     kDashboardAccent, TextAlign::Left, 3);
    surface.drawText(30, 202, calm_grid::fitText(surface, snapshot.currentCondition, 92, 1),
                     kDashboardBlack, TextAlign::Left, 1);

    drawWeatherTodayHourlyChart(surface, Rect{148, 58, 234, 166}, snapshot.hourly, tempUnit);

    surface.drawRect(18, 233, 364, 45, kDashboardBlack);
    surface.drawText(28, 240, "WEEKLY WEATHER", kDashboardAccent, TextAlign::Left, 1);
    surface.drawLine(28, 250, 372, 250, kDashboardBlack);
    drawCompactWeeklyForecast(surface, Rect{28, 252, 344, 28}, weatherTodayWindow(snapshot.weekly),
                              tempUnit);
}

void renderWeeklyWeatherPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot,
                             size_t pageNumber, size_t pageCount) {
    calm_grid::drawPrototypePageChrome(surface, "WEEKLY WEATHER",
                                       calm_grid::PageIconKind::Weather, pageNumber, pageCount);
    const std::string tempUnit = normalizeTempUnit(snapshot.tempUnit);
    std::vector<WeatherDayCell> days = weeklyWeatherWindow(snapshot.weekly);
    drawWeeklyWeatherTrend(surface, Rect{18, 62, 364, 113}, days, tempUnit);
    drawWeeklyWeatherSummary(surface, Rect{18, 183, 364, 85}, days, tempUnit);
}

void renderIndoorClimatePage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot,
                             size_t pageNumber, size_t pageCount) {
    calm_grid::drawPrototypePageChrome(surface, "INDOOR CLIMATE",
                                       calm_grid::PageIconKind::Weather, pageNumber, pageCount);
    const std::string tempUnit = normalizeTempUnit(snapshot.tempUnit);
    drawWeatherMetricCard(surface, Rect{18, 64, 176, 58}, "Indoor",
                          tempTextFromC(snapshot.indoorTempC, tempUnit));
    drawWeatherMetricCard(surface, Rect{206, 64, 176, 58}, "Humidity",
                          std::to_string(snapshot.indoorHumidityPct) + "%");
    drawWeatherMetricCard(surface, Rect{18, 138, 176, 58}, "Outdoor",
                          snapshot.currentCondition);
    drawWeatherMetricCard(surface, Rect{206, 138, 176, 58}, "Feels",
                          tempTextFromC(snapshot.feelsLikeC, tempUnit));
    surface.drawRect(18, 216, 364, 34, kDashboardBlack);
    surface.drawText(28, 237,
                     calm_grid::fitText(surface, "AHT20 local sensor values stay on device.", 360, 1),
                     kDashboardBlack, TextAlign::Left, 1);
}
