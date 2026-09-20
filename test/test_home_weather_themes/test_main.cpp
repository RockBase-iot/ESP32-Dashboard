#include <unity.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "ui/canvas/draw_surface.h"
#include "ui/components/calm_grid.h"
#include "app/weather/home_weather_snapshot.h"
#include "ui/layouts/epd_400x300/render_home_weather.h"
#include "ui/components/calm_grid.cpp"
#include "app/weather/home_weather_snapshot.cpp"
#include "ui/layouts/epd_400x300/render_home_weather.cpp"

namespace {
class ProbeSurface final : public IDrawSurface {
public:
    struct TextDraw {
        std::string text;
        int16_t x;
        int16_t y;
        uint8_t size;
    };

    int16_t width() const override { return 400; }
    int16_t height() const override { return 300; }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if (x < 0 || y < 0 || x >= width() || y >= height()) {
            ++outOfBounds;
            return;
        }
        if (color == kDashboardBlack) ++blackPixels;
        if (color == kDashboardBlack && x >= 14 && x <= 385 && y >= 146 && y <= 215) {
            ++rhythmGraphInkPixels;
            rhythmGraphInkTrace.push_back((static_cast<uint32_t>(y) << 16) |
                                          static_cast<uint16_t>(x));
        }
        if (color == kDashboardAccent) ++redPixels;
        if (color == kDashboardHighlight) ++yellowPixels;
        if ((color == kDashboardAccent || color == kDashboardHighlight) && x <= 2 &&
            y >= 70 && y <= 220) {
            ++leftEdgeChromaPixels;
        }
        if ((color == kDashboardAccent || color == kDashboardHighlight) && x >= 220 &&
            y >= 60 && y <= 250) {
            ++rightFieldChromaPixels;
        }
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override {
        const int16_t dx = std::abs(static_cast<int>(x1 - x0));
        const int16_t sx = x0 < x1 ? 1 : -1;
        const int16_t dy = -std::abs(static_cast<int>(y1 - y0));
        const int16_t sy = y0 < y1 ? 1 : -1;
        int16_t error = static_cast<int16_t>(dx + dy);
        while (true) {
            drawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            const int16_t doubled = static_cast<int16_t>(2 * error);
            if (doubled >= dy) {
                error = static_cast<int16_t>(error + dy);
                x0 = static_cast<int16_t>(x0 + sx);
            }
            if (doubled <= dx) {
                error = static_cast<int16_t>(error + dx);
                y0 = static_cast<int16_t>(y0 + sy);
            }
        }
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        drawLine(x, y, static_cast<int16_t>(x + w - 1), y, color);
        drawLine(x, static_cast<int16_t>(y + h - 1),
                 static_cast<int16_t>(x + w - 1), static_cast<int16_t>(y + h - 1), color);
        drawLine(x, y, x, static_cast<int16_t>(y + h - 1), color);
        drawLine(static_cast<int16_t>(x + w - 1), y,
                 static_cast<int16_t>(x + w - 1), static_cast<int16_t>(y + h - 1), color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        for (int16_t row = 0; row < h; ++row) {
            for (int16_t column = 0; column < w; ++column) {
                drawPixel(static_cast<int16_t>(x + column), static_cast<int16_t>(y + row), color);
            }
        }
    }

    void fillScreen(uint16_t color) override { fillRect(0, 0, width(), height(), color); }

    void drawText(int16_t x, int16_t y, const std::string &text, uint16_t color,
                  TextAlign align, uint8_t size) override {
        texts.push_back(text);
        textDraws.push_back({text, x, y, size});
        int16_t drawX = x;
        const int16_t textWidth = measureText(text, size);
        if (align == TextAlign::Center) drawX = static_cast<int16_t>(x - textWidth / 2);
        if (align == TextAlign::Right) drawX = static_cast<int16_t>(x - textWidth);
        fillRect(drawX, static_cast<int16_t>(y - 8 * size + 1), textWidth,
                 static_cast<int16_t>(8 * size), color);
    }

    int16_t measureText(const std::string &text, uint8_t size) const override {
        return static_cast<int16_t>(text.size() * 6 * size);
    }

    bool hasText(const std::string &text) const {
        return std::find(texts.begin(), texts.end(), text) != texts.end();
    }

    bool hasTextAt(const std::string &text, int16_t x, int16_t y, uint8_t size) const {
        return std::any_of(textDraws.begin(), textDraws.end(),
                           [&text, x, y, size](const TextDraw &draw) {
                               return draw.text == text && draw.x == x && draw.y == y &&
                                      draw.size == size;
                           });
    }

    size_t blackPixels = 0;
    size_t rhythmGraphInkPixels = 0;
    std::vector<uint32_t> rhythmGraphInkTrace;
    size_t redPixels = 0;
    size_t yellowPixels = 0;
    size_t leftEdgeChromaPixels = 0;
    size_t rightFieldChromaPixels = 0;
    size_t outOfBounds = 0;
    std::vector<std::string> texts;
    std::vector<TextDraw> textDraws;
};

HomeWeatherSnapshot sampleSnapshot() {
    HomeWeatherSnapshot snapshot;
    snapshot.valid = true;
    snapshot.location = "CHENGDU";
    snapshot.condition = "PARTLY CLOUDY";
    snapshot.temperatureUnit = "C";
    snapshot.windUnit = "m/s";
    snapshot.currentTempC = 21.0f;
    snapshot.lowTempC = 18.0f;
    snapshot.highTempC = 26.0f;
    snapshot.cloudCoverPct = 62.0f;
    snapshot.currentPrecipitationMm = 0.0f;
    snapshot.windMps = 3.6f;
    snapshot.windDisplayValue = 3.6f;
    snapshot.isDay = true;
    for (int hour = 10; hour <= 21; ++hour) {
        HomeWeatherHour point;
        point.timeLabel = std::to_string(hour);
        point.temperatureC = 21.0f + static_cast<float>((hour - 10) % 6);
        point.precipitationMm = hour >= 18 ? 0.3f : 0.0f;
        snapshot.hourly.push_back(point);
    }
    return snapshot;
}

HomeWeatherPalette threeColorPalette() {
    return HomeWeatherPalette{kDashboardWhite, kDashboardBlack, kDashboardAccent,
                              kDashboardWhite, false};
}

HomeWeatherPalette fourColorPalette() {
    return HomeWeatherPalette{kDashboardWhite, kDashboardBlack, kDashboardAccent,
                              kDashboardHighlight, true};
}
}  // namespace

void test_atlas_uses_shared_geometry_and_palette_specific_pigments() {
    ProbeSurface threeColor;
    ProbeSurface fourColor;

    renderHomeAtlasPage(threeColor, sampleSnapshot(), threeColorPalette(), 17, 19, "IP: 10.0.0.8");
    renderHomeAtlasPage(fourColor, sampleSnapshot(), fourColorPalette(), 17, 19, "IP: 10.0.0.8");

    TEST_ASSERT_TRUE(threeColor.hasText("HOME - ATLAS"));
    TEST_ASSERT_TRUE(threeColor.hasTextAt("21", 204, 102, 5));
    TEST_ASSERT_TRUE(threeColor.hasText("18-26"));
    TEST_ASSERT_TRUE(threeColor.hasText("C"));
    TEST_ASSERT_TRUE(threeColor.hasText(" - 24H"));
    TEST_ASSERT_TRUE(threeColor.hasText("RAIN FROM 18:00"));
    TEST_ASSERT_EQUAL_UINT(0, threeColor.yellowPixels);
    TEST_ASSERT_GREATER_THAN_UINT(0, threeColor.redPixels);
    TEST_ASSERT_GREATER_THAN_UINT(0, fourColor.yellowPixels);
    TEST_ASSERT_GREATER_THAN_UINT(0, threeColor.leftEdgeChromaPixels);
    TEST_ASSERT_GREATER_THAN_UINT(0, fourColor.leftEdgeChromaPixels);
    TEST_ASSERT_EQUAL_UINT(0, threeColor.outOfBounds);
    TEST_ASSERT_EQUAL_UINT(0, fourColor.outOfBounds);
}

void test_print_engraving_responds_to_cloud_and_wind() {
    ProbeSurface surface;
    renderHomePrintPage(surface, sampleSnapshot(), fourColorPalette(), 18, 19, "IP: --");

    TEST_ASSERT_TRUE(surface.hasText("HOME - PRINT"));
    TEST_ASSERT_TRUE(surface.hasTextAt("21", 12, 108, 6));
    TEST_ASSERT_TRUE(surface.hasText("C"));
    TEST_ASSERT_TRUE(surface.hasTextAt("RAIN FROM 18:00 - WIND 3.6 m/s", 14, 260, 1));
    TEST_ASSERT_GREATER_THAN_UINT(0, surface.rightFieldChromaPixels);
    TEST_ASSERT_EQUAL_UINT(0, surface.outOfBounds);
}

void test_rhythm_draws_future_precipitation_bars() {
    HomeWeatherSnapshot dry = sampleSnapshot();
    for (HomeWeatherHour &hour : dry.hourly) hour.precipitationMm = 0.0f;
    ProbeSurface drySurface;
    ProbeSurface futureRainSurface;

    renderHomeRhythmPage(drySurface, dry, fourColorPalette(), 16, 19, "IP: --");
    renderHomeRhythmPage(futureRainSurface, sampleSnapshot(), fourColorPalette(), 16, 19, "IP: --");

    TEST_ASSERT_TRUE(futureRainSurface.hasText("HOME - RHYTHM"));
    TEST_ASSERT_TRUE(futureRainSurface.hasTextAt("21", 14, 108, 5));
    TEST_ASSERT_FALSE(futureRainSurface.hasText("NEXT HOURS - C / mm"));
    TEST_ASSERT_GREATER_THAN_UINT(drySurface.blackPixels, futureRainSurface.blackPixels);
}

void test_temperature_units_convert_to_fahrenheit_for_all_home_themes() {
    HomeWeatherSnapshot snapshot = sampleSnapshot();
    snapshot.temperatureUnit = "F";
    ProbeSurface rhythm;
    ProbeSurface atlas;
    ProbeSurface print;

    renderHomeRhythmPage(rhythm, snapshot, fourColorPalette());
    renderHomeAtlasPage(atlas, snapshot, fourColorPalette());
    renderHomePrintPage(print, snapshot, fourColorPalette());

    for (const ProbeSurface *surface : {&rhythm, &atlas, &print}) {
        TEST_ASSERT_TRUE(surface->hasText("70"));
        TEST_ASSERT_TRUE(surface->hasText("64-79"));
        TEST_ASSERT_TRUE(surface->hasText("F"));
    }
}

void test_rhythm_graph_ignores_missing_leading_temperature() {
    HomeWeatherSnapshot snapshot = sampleSnapshot();
    for (HomeWeatherHour &hour : snapshot.hourly) hour.precipitationMm = 0.0f;
    HomeWeatherSnapshot expected = snapshot;
    snapshot.hourly.front().temperatureC = NAN;
    ProbeSurface expectedSurface;
    ProbeSurface surface;

    renderHomeRhythmPage(expectedSurface, expected, fourColorPalette(), 16, 19, "IP: --");
    renderHomeRhythmPage(surface, snapshot, fourColorPalette(), 16, 19, "IP: --");

    TEST_ASSERT_GREATER_THAN_UINT(0, surface.rhythmGraphInkPixels);
    TEST_ASSERT_TRUE(expectedSurface.rhythmGraphInkTrace == surface.rhythmGraphInkTrace);
    TEST_ASSERT_EQUAL_UINT(0, surface.outOfBounds);
}

void test_current_rain_strokes_depend_only_on_current_precipitation() {
    HomeWeatherSnapshot raining = sampleSnapshot();
    raining.currentPrecipitationMm = 1.2f;
    ProbeSurface drySurface;
    ProbeSurface rainingSurface;

    renderHomeAtlasPage(drySurface, sampleSnapshot(), fourColorPalette(), 17, 19, "IP: --");
    renderHomeAtlasPage(rainingSurface, raining, fourColorPalette(), 17, 19, "IP: --");

    TEST_ASSERT_GREATER_THAN_UINT(drySurface.blackPixels, rainingSurface.blackPixels);
}

void test_page_ids_map_to_independent_home_weather_themes() {
    HomeWeatherTheme theme = HomeWeatherTheme::Rhythm;
    TEST_ASSERT_TRUE(homeWeatherThemeForPage(PageId::HomeRhythm, theme));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HomeWeatherTheme::Rhythm),
                            static_cast<uint8_t>(theme));
    TEST_ASSERT_TRUE(homeWeatherThemeForPage(PageId::HomeAtlas, theme));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HomeWeatherTheme::Atlas),
                            static_cast<uint8_t>(theme));
    TEST_ASSERT_TRUE(homeWeatherThemeForPage(PageId::HomePrint, theme));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HomeWeatherTheme::Print),
                            static_cast<uint8_t>(theme));
    TEST_ASSERT_FALSE(homeWeatherThemeForPage(PageId::WeatherToday, theme));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_atlas_uses_shared_geometry_and_palette_specific_pigments);
    RUN_TEST(test_print_engraving_responds_to_cloud_and_wind);
    RUN_TEST(test_rhythm_draws_future_precipitation_bars);
    RUN_TEST(test_temperature_units_convert_to_fahrenheit_for_all_home_themes);
    RUN_TEST(test_rhythm_graph_ignores_missing_leading_temperature);
    RUN_TEST(test_current_rain_strokes_depend_only_on_current_precipitation);
    RUN_TEST(test_page_ids_map_to_independent_home_weather_themes);
    UNITY_END();
}

void loop() {}
