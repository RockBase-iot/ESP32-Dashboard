#include "render_home_weather.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
enum class LogicalPigment : uint8_t {
    Paper,
    Ink,
    Yellow,
    Red,
};

constexpr int16_t kBodyTop = 48;
constexpr int16_t kBodyBottom = 270;
constexpr uint8_t kBayer4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

float finiteOr(float value, float fallback) {
    return std::isfinite(value) ? value : fallback;
}

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

LogicalPigment mixPigment(int16_t x, int16_t y, LogicalPigment first,
                          LogicalPigment second, float amount) {
    const float threshold = static_cast<float>(kBayer4[y & 3][x & 3]) / 15.0f;
    return threshold < clamp01(amount) ? second : first;
}

uint16_t physicalColor(LogicalPigment pigment, int16_t x, int16_t y,
                       const HomeWeatherPalette &palette) {
    if (pigment == LogicalPigment::Paper) return palette.paper;
    if (pigment == LogicalPigment::Ink) return palette.ink;
    if (palette.hasYellow) {
        return pigment == LogicalPigment::Yellow ? palette.yellow : palette.red;
    }
    const uint8_t threshold = kBayer4[y & 3][x & 3];
    if (pigment == LogicalPigment::Yellow) {
        return threshold < 4 ? palette.red : palette.paper;
    }
    return threshold < 12 ? palette.red : palette.paper;
}

void logicalPixel(IDrawSurface &surface, int16_t x, int16_t y, LogicalPigment pigment,
                  const HomeWeatherPalette &palette) {
    if (x < 0 || x >= surface.width() || y < kBodyTop || y > kBodyBottom ||
        y >= surface.height()) {
        return;
    }
    surface.drawPixel(x, y, physicalColor(pigment, x, y, palette));
}

void logicalLine(IDrawSurface &surface, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                 LogicalPigment pigment, const HomeWeatherPalette &palette) {
    int16_t deltaX = static_cast<int16_t>(std::abs(static_cast<int>(x1 - x0)));
    const int16_t stepX = x0 < x1 ? 1 : -1;
    int16_t deltaY = static_cast<int16_t>(-std::abs(static_cast<int>(y1 - y0)));
    const int16_t stepY = y0 < y1 ? 1 : -1;
    int16_t error = static_cast<int16_t>(deltaX + deltaY);
    while (true) {
        logicalPixel(surface, x0, y0, pigment, palette);
        if (x0 == x1 && y0 == y1) break;
        const int16_t doubled = static_cast<int16_t>(2 * error);
        if (doubled >= deltaY) {
            error = static_cast<int16_t>(error + deltaY);
            x0 = static_cast<int16_t>(x0 + stepX);
        }
        if (doubled <= deltaX) {
            error = static_cast<int16_t>(error + deltaX);
            y0 = static_cast<int16_t>(y0 + stepY);
        }
    }
}

int displayTemperature(float celsius, const HomeWeatherSnapshot &snapshot) {
    const float display = snapshot.temperatureUnit == "F" ? celsius * 9.0f / 5.0f + 32.0f : celsius;
    return static_cast<int>(std::round(display));
}

std::string temperatureUnit(const HomeWeatherSnapshot &snapshot) {
    return snapshot.temperatureUnit == "F" ? "F" : "C";
}

std::string currentTemperatureText(const HomeWeatherSnapshot &snapshot) {
    if (!std::isfinite(snapshot.currentTempC)) return "--";
    return std::to_string(displayTemperature(snapshot.currentTempC, snapshot));
}

std::string rangeText(const HomeWeatherSnapshot &snapshot) {
    if (!std::isfinite(snapshot.lowTempC) || !std::isfinite(snapshot.highTempC)) {
        return "--";
    }
    return std::to_string(displayTemperature(snapshot.lowTempC, snapshot)) + "-" +
           std::to_string(displayTemperature(snapshot.highTempC, snapshot));
}

uint8_t temperatureUnitTextSize(uint8_t temperatureTextSize) {
    return temperatureTextSize >= 5 ? 2 : 1;
}

int16_t degreeDiameter(uint8_t temperatureTextSize) {
    return temperatureTextSize >= 5 ? 7 : 5;
}

int16_t degreeUnitWidth(const IDrawSurface &surface, const std::string &unit,
                        uint8_t temperatureTextSize) {
    const uint8_t unitSize = temperatureUnitTextSize(temperatureTextSize);
    return static_cast<int16_t>(degreeDiameter(temperatureTextSize) + 2 +
                                surface.measureText(unit, unitSize));
}

void drawDegreeUnit(IDrawSurface &surface, int16_t x, int16_t baselineY,
                    const std::string &unit, uint16_t color, uint8_t temperatureTextSize) {
    const int16_t diameter = degreeDiameter(temperatureTextSize);
    const int16_t radius = diameter / 2;
    const int16_t centerX = static_cast<int16_t>(x + radius);
    const int16_t centerY = static_cast<int16_t>(baselineY - 8 * temperatureTextSize + radius + 1);
    const int16_t outerRadiusSquared = radius * radius;
    const int16_t innerRadiusSquared = std::max<int16_t>(0, (radius - 1) * (radius - 1));
    for (int16_t offsetY = -radius; offsetY <= radius; ++offsetY) {
        for (int16_t offsetX = -radius; offsetX <= radius; ++offsetX) {
            const int16_t distanceSquared = offsetX * offsetX + offsetY * offsetY;
            if (distanceSquared <= outerRadiusSquared && distanceSquared >= innerRadiusSquared) {
                surface.drawPixel(static_cast<int16_t>(centerX + offsetX),
                                  static_cast<int16_t>(centerY + offsetY), color);
            }
        }
    }
    const uint8_t unitSize = temperatureUnitTextSize(temperatureTextSize);
    surface.drawText(static_cast<int16_t>(x + diameter + 2),
                     static_cast<int16_t>(baselineY - 8 * temperatureTextSize + 8 * unitSize),
                     unit, color, TextAlign::Left, unitSize);
}

void drawCurrentTemperature(IDrawSurface &surface, int16_t x, int16_t baselineY,
                            const HomeWeatherSnapshot &snapshot, uint16_t color,
                            uint8_t temperatureTextSize, int16_t degreeUnitOffsetY = 0) {
    const std::string temperature = currentTemperatureText(snapshot);
    surface.drawText(x, baselineY, temperature, color, TextAlign::Left, temperatureTextSize);
    if (temperature != "--") {
        drawDegreeUnit(surface,
                       static_cast<int16_t>(x + surface.measureText(temperature, temperatureTextSize) + 2),
                       static_cast<int16_t>(baselineY + degreeUnitOffsetY),
                       temperatureUnit(snapshot), color, temperatureTextSize);
    }
}

void drawTemperatureRange(IDrawSurface &surface, int16_t x, int16_t baselineY,
                          const HomeWeatherSnapshot &snapshot, uint16_t color) {
    const std::string range = rangeText(snapshot);
    surface.drawText(x, baselineY, range, color, TextAlign::Left, 1);
    if (range == "--") {
        surface.drawText(static_cast<int16_t>(x + surface.measureText(range, 1)), baselineY,
                         " - 24H", color, TextAlign::Left, 1);
        return;
    }
    const int16_t unitX = static_cast<int16_t>(x + surface.measureText(range, 1) + 1);
    const std::string unit = temperatureUnit(snapshot);
    drawDegreeUnit(surface, unitX, baselineY + 2, unit, color, 1);
    surface.drawText(static_cast<int16_t>(unitX + degreeUnitWidth(surface, unit, 1)), baselineY,
                     " - 24H", color, TextAlign::Left, 1);
}

std::string windText(const HomeWeatherSnapshot &snapshot) {
    if (!std::isfinite(snapshot.windDisplayValue)) return "WIND --";
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "WIND %.1f %s", snapshot.windDisplayValue,
                  snapshot.windUnit.c_str());
    return buffer;
}

void drawDisc(IDrawSurface &surface, int16_t centerX, int16_t centerY, int16_t radiusX,
              int16_t radiusY, const HomeWeatherSnapshot &snapshot,
              const HomeWeatherPalette &palette) {
    const float cloud = clamp01(finiteOr(snapshot.cloudCoverPct, 0.0f) / 100.0f);
    const float warmth = clamp01((finiteOr(snapshot.currentTempC, 10.0f) + 15.0f) / 55.0f);
    const float inverseRadiusX = 1.0f / radiusX;
    const float inverseRadiusY = 1.0f / radiusY;
    const int16_t left = std::max<int16_t>(0, centerX - radiusX);
    const int16_t right = std::min<int16_t>(surface.width() - 1, centerX + radiusX);
    const int16_t top = std::max<int16_t>(kBodyTop, centerY - radiusY);
    const int16_t bottom = std::min<int16_t>(kBodyBottom, centerY + radiusY);

    for (int16_t x = left; x <= right; ++x) {
        const float deltaX = (x - centerX) * inverseRadiusX;
        const float shade = std::clamp((deltaX + 1.0f) * 0.35f + warmth * 0.3f, 0.04f, 0.96f);
        const float nightShade = 0.12f + 0.35f * (deltaX + 1.0f);
        const float edge = 0.54f - cloud * 1.05f + 0.10f * std::sin((x - centerX) / 15.0f);
        const float obscuredShade = cloud * 0.36f;
        for (int16_t y = top; y <= bottom; ++y) {
            const float deltaY = (y - centerY) * inverseRadiusY;
            const float radius = deltaX * deltaX + deltaY * deltaY;
            if (radius > 1.0f) continue;

            LogicalPigment pigment = snapshot.isDay
                                          ? mixPigment(x, y, LogicalPigment::Yellow,
                                                       LogicalPigment::Red, shade)
                                          : mixPigment(x, y, LogicalPigment::Paper,
                                                       LogicalPigment::Ink, nightShade);
            if (deltaX < -0.35f && radius > 0.36f &&
                static_cast<int>(std::sqrt(radius) * 40.0f) % 7 == 0) {
                pigment = snapshot.isDay ? LogicalPigment::Yellow : LogicalPigment::Paper;
            }
            if (deltaY > edge) {
                pigment = mixPigment(x, y, LogicalPigment::Paper,
                                     LogicalPigment::Ink, obscuredShade);
            }
            logicalPixel(surface, x, y, pigment, palette);
        }
    }
}

void drawRainField(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                   int16_t top, int16_t bottom, const HomeWeatherPalette &palette) {
    const float precipitation = std::clamp(finiteOr(snapshot.currentPrecipitationMm, 0.0f),
                                           0.0f, 20.0f);
    const float wind = std::clamp(finiteOr(snapshot.windMps, 0.0f), 0.0f, 40.0f);
    const int rainRows = std::clamp(static_cast<int>(precipitation * 1.8f), 1, 7);
    const bool raining = homeCurrentRainActive(snapshot);
    const float waveScale = 1.0f / (46.0f + wind * 2.0f);

    for (int16_t x = 0; x < surface.width(); ++x) {
        const int16_t horizon = static_cast<int16_t>(top + 5 + 4 * std::sin(x * waveScale));
        const float inverseDepth = 1.0f / std::max<int16_t>(1, bottom - horizon);
        const float warmth = static_cast<float>(x) / std::max<int16_t>(1, surface.width() - 1) * 0.6f;
        for (int16_t y = top; y < bottom; ++y) {
            if (y >= horizon) {
                const float depth = (y - horizon) * inverseDepth;
                LogicalPigment pigment = mixPigment(x, y, LogicalPigment::Paper,
                                                    LogicalPigment::Yellow, depth * 0.95f);
                if (pigment == LogicalPigment::Yellow) {
                    pigment = mixPigment(x, y, LogicalPigment::Yellow,
                                         LogicalPigment::Red, depth * warmth);
                }
                logicalPixel(surface, x, y, pigment, palette);
            }
            const int windShift = static_cast<int>(wind * y / 12.0f);
            if (raining && (x + windShift) % 13 == 0 && (y - top) % 9 < rainRows) {
                logicalPixel(surface, x, y, LogicalPigment::Ink, palette);
            }
        }
    }
}

void drawRhythmGraph(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                     const HomeWeatherPalette &palette) {
    constexpr int16_t x0 = 14;
    constexpr int16_t y0 = 146;
    constexpr int16_t width = 372;
    constexpr int16_t height = 70;
    if (snapshot.hourly.size() < 2) return;

    float low = 0.0f;
    float high = 0.0f;
    bool hasFiniteTemperature = false;
    for (const HomeWeatherHour &hour : snapshot.hourly) {
        if (!std::isfinite(hour.temperatureC)) continue;
        if (!hasFiniteTemperature) {
            low = hour.temperatureC;
            high = hour.temperatureC;
            hasFiniteTemperature = true;
        } else {
            low = std::min(low, hour.temperatureC);
            high = std::max(high, hour.temperatureC);
        }
    }
    if (!hasFiniteTemperature) return;
    const float span = std::max(2.0f, high - low);
    int16_t previousX = x0;
    int16_t previousY = static_cast<int16_t>(y0 + height / 2);
    for (int16_t offset = 0; offset < width; ++offset) {
        const float position = static_cast<float>(offset) * (snapshot.hourly.size() - 1) /
                               (width - 1);
        const size_t leftIndex = std::min(static_cast<size_t>(position),
                                          snapshot.hourly.size() - 2);
        const float fraction = position - static_cast<float>(leftIndex);
        const float leftValue = finiteOr(snapshot.hourly[leftIndex].temperatureC, low);
        const float rightValue = finiteOr(snapshot.hourly[leftIndex + 1].temperatureC, leftValue);
        const float value = leftValue * (1.0f - fraction) + rightValue * fraction;
        const int16_t pointY = static_cast<int16_t>(
            y0 + height - 1 - (value - low) / span * (height - 1));
        const float shadeStep = 0.78f / std::max<int16_t>(1, y0 + height - pointY);
        for (int16_t fillY = pointY; fillY < y0 + height; ++fillY) {
            logicalPixel(surface, static_cast<int16_t>(x0 + offset), fillY,
                         mixPigment(static_cast<int16_t>(x0 + offset), fillY,
                                    LogicalPigment::Yellow, LogicalPigment::Red,
                                    (fillY - pointY) * shadeStep), palette);
        }
        if (offset > 0) {
            logicalLine(surface, previousX, previousY, static_cast<int16_t>(x0 + offset),
                        pointY, LogicalPigment::Ink, palette);
        }
        previousX = static_cast<int16_t>(x0 + offset);
        previousY = pointY;
    }

    for (size_t index = 0; index < snapshot.hourly.size(); ++index) {
        const HomeWeatherHour &hour = snapshot.hourly[index];
        if (!homeHourlyRainActive(hour)) continue;
        const int16_t centerX = static_cast<int16_t>(
            x0 + index * (width - 1) / std::max<size_t>(1, snapshot.hourly.size() - 1));
        const int16_t bars = std::min<int16_t>(
            static_cast<int16_t>(std::max(1.0f, hour.precipitationMm * 3.0f)), height / 2);
        for (int16_t barY = static_cast<int16_t>(y0 + height - bars);
             barY < y0 + height; ++barY) {
            for (int16_t deltaX = -3; deltaX <= 3; ++deltaX) {
                logicalPixel(surface, static_cast<int16_t>(centerX + deltaX), barY,
                             LogicalPigment::Ink, palette);
            }
        }
    }
}

void drawPrintEngraving(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                        const HomeWeatherPalette &palette) {
    constexpr int16_t x0 = 184;
    constexpr int16_t y0 = 48;
    constexpr int16_t width = 216;
    constexpr int16_t height = 163;
    const float cloud = clamp01(finiteOr(snapshot.cloudCoverPct, 0.0f) / 100.0f);
    const float wind = std::clamp(finiteOr(snapshot.windMps, 0.0f), 0.0f, 50.0f);
    for (int16_t x = x0; x < x0 + width; ++x) {
        const float coverage = static_cast<float>(x - x0) / width * (0.12f + cloud * 0.4f);
        const int wave = static_cast<int>(wind * 2.0f * std::sin(x / 61.0f));
        for (int16_t y = y0; y < y0 + height; ++y) {
            LogicalPigment pigment = mixPigment(x, y, LogicalPigment::Paper,
                                                LogicalPigment::Yellow, coverage);
            if ((y + wave) % 13 == 0) {
                pigment = mixPigment(x, y, LogicalPigment::Paper,
                                     LogicalPigment::Yellow, 0.72f);
            }
            logicalPixel(surface, x, y, pigment, palette);
        }
    }
}

void drawRhythm(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                const HomeWeatherPalette &palette) {
    surface.drawText(14, 66, snapshot.location + " - FORECAST", palette.red,
                     TextAlign::Left, 1);
    drawCurrentTemperature(surface, 14, 95, snapshot, palette.ink, 5, 15);
    surface.drawText(148, 86, snapshot.condition, palette.ink, TextAlign::Left, 2);
    drawTemperatureRange(surface, 148, 106, snapshot, palette.ink);
    const float cloud = clamp01(finiteOr(snapshot.cloudCoverPct, 0.0f) / 100.0f);
    for (int16_t x = 0; x < surface.width(); ++x) {
        const float coverage = cloud * static_cast<float>(x) / surface.width() * 0.5f;
        for (int16_t y = 141; y < 216; ++y) {
            logicalPixel(surface, x, y,
                         mixPigment(x, y, LogicalPigment::Paper,
                                    LogicalPigment::Yellow, coverage), palette);
        }
    }
    drawRhythmGraph(surface, snapshot, palette);
    surface.drawText(14, 254, homeRainOutlook(snapshot) + " - " + windText(snapshot),
                     palette.ink, TextAlign::Left, 1);
}

void drawAtlas(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
               const HomeWeatherPalette &palette) {
    for (int16_t y = kBodyTop; y < 250; ++y) {
        for (int16_t x = 0; x < 196; ++x) {
            if ((x + y) % 17 < 2) {
                logicalPixel(surface, x, y, LogicalPigment::Yellow, palette);
            }
        }
    }
    drawDisc(surface, 90, 132, 102, 99, snapshot, palette);
    drawRainField(surface, snapshot, 218, 258, palette);
    surface.fillRect(196, 48, 204, 169, palette.paper);
    surface.fillRect(194, 210, 206, 48, palette.paper);
    surface.drawText(208, 66, snapshot.location + " - FORECAST", palette.red,
                     TextAlign::Left, 1);
    drawCurrentTemperature(surface, 204, 95, snapshot, palette.ink, 5, 15);
    drawTemperatureRange(surface, 208, 145, snapshot, palette.ink);
    surface.drawText(208, 174, snapshot.condition, palette.ink, TextAlign::Left, 2);
    surface.drawText(208, 232, homeRainOutlook(snapshot), palette.ink, TextAlign::Left, 1);
    surface.drawText(208, 252, windText(snapshot), palette.ink, TextAlign::Left, 1);
}

void drawPrint(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
               const HomeWeatherPalette &palette) {
    drawPrintEngraving(surface, snapshot, palette);
    drawDisc(surface, 300, 119, 89, 78, snapshot, palette);
    drawRainField(surface, snapshot, 218, 258, palette);
    surface.drawText(14, 66, snapshot.location + " - FORECAST", palette.red,
                     TextAlign::Left, 1);
    drawCurrentTemperature(surface, 12, 95, snapshot, palette.ink, 6, 15);
    drawTemperatureRange(surface, 14, 151, snapshot, palette.ink);
    const size_t split = snapshot.condition.find(' ');
    const std::string firstLine = split == std::string::npos
                                      ? snapshot.condition
                                      : snapshot.condition.substr(0, split);
    const std::string secondLine = split == std::string::npos
                                       ? ""
                                       : snapshot.condition.substr(split + 1);
    surface.drawText(14, 184, firstLine, palette.ink, TextAlign::Left, 2);
    if (!secondLine.empty()) {
        surface.drawText(14, 208, secondLine, palette.ink, TextAlign::Left, 2);
    }
    surface.drawText(14, 260, homeRainOutlook(snapshot) + " - " + windText(snapshot),
                     palette.ink, TextAlign::Left, 1);
}
}  // namespace

bool homeWeatherThemeForPage(PageId page, HomeWeatherTheme &theme) {
    if (page == PageId::HomeRhythm) {
        theme = HomeWeatherTheme::Rhythm;
        return true;
    }
    if (page == PageId::HomeAtlas) {
        theme = HomeWeatherTheme::Atlas;
        return true;
    }
    if (page == PageId::HomePrint) {
        theme = HomeWeatherTheme::Print;
        return true;
    }
    return false;
}

void renderHomeWeatherPage(IDrawSurface &surface, HomeWeatherTheme theme,
                           const HomeWeatherSnapshot &snapshot,
                           const HomeWeatherPalette &palette, size_t pageNumber,
                           size_t pageCount, const std::string &ipText,
                           calm_grid::ChromeContext chrome) {
    const char *title = theme == HomeWeatherTheme::Rhythm
                                                ? "HOME - RHYTHM"
                                                : (theme == HomeWeatherTheme::Atlas ? "HOME - ATLAS" : "HOME - PRINT");
    calm_grid::drawPrototypePageChrome(surface, title, calm_grid::PageIconKind::Weather,
                                       pageNumber, pageCount, chrome.timeText,
                                       ipText.empty() ? chrome.ipText : ipText,
                                       chrome.batteryText);
    if (!snapshot.valid) {
        surface.drawText(surface.width() / 2, 150, "WEATHER UNAVAILABLE", palette.ink,
                         TextAlign::Center, 2);
        return;
    }
    if (theme == HomeWeatherTheme::Rhythm) {
        drawRhythm(surface, snapshot, palette);
    } else if (theme == HomeWeatherTheme::Atlas) {
        drawAtlas(surface, snapshot, palette);
    } else {
        drawPrint(surface, snapshot, palette);
    }
}

void renderHomeRhythmPage(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                          const HomeWeatherPalette &palette, size_t pageNumber,
                          size_t pageCount, const std::string &ipText,
                          calm_grid::ChromeContext chrome) {
    renderHomeWeatherPage(surface, HomeWeatherTheme::Rhythm, snapshot, palette, pageNumber,
                          pageCount, ipText, chrome);
}

void renderHomeAtlasPage(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                         const HomeWeatherPalette &palette, size_t pageNumber,
                         size_t pageCount, const std::string &ipText,
                         calm_grid::ChromeContext chrome) {
    renderHomeWeatherPage(surface, HomeWeatherTheme::Atlas, snapshot, palette, pageNumber,
                          pageCount, ipText, chrome);
}

void renderHomePrintPage(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                         const HomeWeatherPalette &palette, size_t pageNumber,
                         size_t pageCount, const std::string &ipText,
                         calm_grid::ChromeContext chrome) {
    renderHomeWeatherPage(surface, HomeWeatherTheme::Print, snapshot, palette, pageNumber,
                          pageCount, ipText, chrome);
}
