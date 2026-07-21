#include "render_weather.h"

#include <algorithm>

#include "ui/components/calm_grid.h"

namespace {
constexpr int16_t kWeatherMargin = 8;

void drawWeatherHeader(IDrawSurface &surface, const std::string &title, const std::string &rightText) {
    calm_grid::drawPageHeader(surface, title, rightText, "", 84);
}

void drawWeatherFooter(IDrawSurface &surface, const std::string &leftText, const std::string &rightText) {
    surface.drawText(kWeatherMargin, 286, calm_grid::fitText(surface, leftText, 190, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(392, 286, calm_grid::fitText(surface, rightText, 174, 1),
                     kDashboardBlack, TextAlign::Right, 1);
}

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

const char *weatherForecastGlyph(int code) {
    if (code >= 60 && code < 90) {
        return "r";
    }
    if (code >= 2 && code < 60) {
        return "c";
    }
    return "*";
}

void drawWeatherForecastStrip(IDrawSurface &surface, const Rect &rect,
                              const std::vector<WeatherDayCell> &days) {
    const size_t count = std::min<size_t>(6, days.size());
    if (count == 0) {
        return;
    }
    const int16_t gap = 2;
    const int16_t cellW = static_cast<int16_t>((rect.w - gap * static_cast<int16_t>(count - 1)) /
                                               static_cast<int16_t>(count));
    for (size_t i = 0; i < count; ++i) {
        const WeatherDayCell &day = days[i];
        const int16_t x = static_cast<int16_t>(rect.x + static_cast<int16_t>(i) * (cellW + gap));
        const Rect cell{x, rect.y, cellW, rect.h};
        surface.drawRect(cell.x, cell.y, cell.w, cell.h, kDashboardBlack);
        surface.drawText(static_cast<int16_t>(cell.x + cell.w / 2),
                         static_cast<int16_t>(cell.y + 13), day.label,
                         kDashboardBlack, TextAlign::Center, 1);
        surface.drawText(static_cast<int16_t>(cell.x + cell.w / 2),
                         static_cast<int16_t>(cell.y + 32), weatherForecastGlyph(day.code),
                         kDashboardBlack, TextAlign::Center, 1);
        surface.drawText(static_cast<int16_t>(cell.x + cell.w / 2),
                         static_cast<int16_t>(cell.y + cell.h - 7),
                         std::to_string(day.highC) + "/" + std::to_string(day.lowC),
                         kDashboardBlack, TextAlign::Center, 1);
    }
}
}  // namespace

WeatherPageSnapshot sampleWeatherPageSnapshot() {
    WeatherPageSnapshot snapshot;
    snapshot.city = "SEATTLE";
    snapshot.updated = "Updated 08:30";
    snapshot.currentCondition = "Sunny";
    snapshot.currentTempC = 72;
    snapshot.feelsLikeC = 71;
    snapshot.humidityPct = 46;
    snapshot.windKph = 4;
    snapshot.rainPct = 10;
    snapshot.indoorTempC = 74;
    snapshot.indoorHumidityPct = 46;
    snapshot.weekly = {
        {"MON", 1, 72, 61},
        {"TUE", 3, 70, 59},
        {"WED", 61, 68, 57},
        {"THU", 1, 74, 60},
        {"FRI", 1, 76, 62},
        {"SAT", 3, 71, 58},
    };
    snapshot.hourly = {
        {"08", 68, 5},
        {"10", 70, 8},
        {"12", 72, 10},
        {"14", 74, 12},
        {"16", 73, 8},
        {"18", 69, 4},
    };
    return snapshot;
}

void renderWeatherTodayPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot) {
    drawWeatherHeader(surface, "WEATHER TODAY", snapshot.city + "  7/12");

    surface.drawText(18, 84, std::to_string(snapshot.currentTempC), kDashboardAccent,
                     TextAlign::Left, 6);
    surface.drawRect(90, 45, 9, 9, kDashboardAccent);
    surface.drawText(18, 110, snapshot.currentCondition, kDashboardBlack, TextAlign::Left, 2);
    surface.drawText(18, 128,
                     calm_grid::fitText(surface,
                         "Feels " + std::to_string(snapshot.feelsLikeC) + "  H:" +
                         std::to_string(snapshot.weekly.empty() ? snapshot.currentTempC
                                                                : snapshot.weekly.front().highC) +
                         " / L:" +
                         std::to_string(snapshot.weekly.empty() ? snapshot.currentTempC
                                                                : snapshot.weekly.front().lowC),
                         132, 1),
                     kDashboardBlack, TextAlign::Left, 1);

    drawWeatherMetricCard(surface, Rect{152, 58, 106, 36}, "Rain",
                          std::to_string(snapshot.rainPct) + "%");
    drawWeatherMetricCard(surface, Rect{264, 58, 118, 36}, "Wind",
                          std::to_string(snapshot.windKph) + " mph");
    drawWeatherMetricCard(surface, Rect{152, 99, 106, 36}, "Indoor",
                          std::to_string(snapshot.indoorTempC) + "F");
    drawWeatherMetricCard(surface, Rect{264, 99, 118, 36}, "Humidity",
                          std::to_string(snapshot.humidityPct) + "%");

    drawWeatherForecastStrip(surface, Rect{18, 151, 364, 55}, snapshot.weekly);
    drawWeatherFooter(surface, "Open-Meteo - No API key", snapshot.updated);
}

void renderWeeklyWeatherPage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot) {
    drawWeatherHeader(surface, "WEEKLY WEATHER", snapshot.city + "  7/12");
    drawWeatherForecastStrip(surface, Rect{18, 64, 364, 70}, snapshot.weekly);
    surface.drawText(18, 164,
                     calm_grid::fitText(surface, "Rain chances stay light through the work week.", 360, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(18, 184,
                     calm_grid::fitText(surface, "Indoor comfort remains steady for home display.", 360, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    drawWeatherFooter(surface, "Open-Meteo - No API key", snapshot.updated);
}

void renderIndoorClimatePage(IDrawSurface &surface, const WeatherPageSnapshot &snapshot) {
    drawWeatherHeader(surface, "INDOOR CLIMATE", snapshot.city + "  7/12");
    drawWeatherMetricCard(surface, Rect{18, 62, 176, 52}, "Indoor",
                          std::to_string(snapshot.indoorTempC) + "F");
    drawWeatherMetricCard(surface, Rect{206, 62, 176, 52}, "Humidity",
                          std::to_string(snapshot.indoorHumidityPct) + "%");
    drawWeatherMetricCard(surface, Rect{18, 126, 176, 52}, "Outdoor",
                          snapshot.currentCondition);
    drawWeatherMetricCard(surface, Rect{206, 126, 176, 52}, "Feels",
                          std::to_string(snapshot.feelsLikeC));
    surface.drawText(18, 210,
                     calm_grid::fitText(surface, "AHT20 local sensor values stay on device.", 360, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    drawWeatherFooter(surface, "Indoor sensor", snapshot.updated);
}
