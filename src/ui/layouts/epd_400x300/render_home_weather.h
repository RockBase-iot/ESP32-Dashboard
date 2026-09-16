#pragma once

#if !defined(UI_LAYOUT_EPD_400x300)
#error "Home weather themes require UI_LAYOUT_EPD_400x300"
#endif

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "app/weather/home_weather_snapshot.h"
#include "app/page/page_catalog.h"
#include "ui/components/calm_grid.h"

enum class HomeWeatherTheme : uint8_t {
    Rhythm,
    Atlas,
    Print,
};

struct HomeWeatherPalette {
    uint16_t paper;
    uint16_t ink;
    uint16_t red;
    uint16_t yellow;
    bool hasYellow;
};

bool homeWeatherThemeForPage(PageId page, HomeWeatherTheme &theme);

void renderHomeWeatherPage(IDrawSurface &surface, HomeWeatherTheme theme,
                           const HomeWeatherSnapshot &snapshot,
                           const HomeWeatherPalette &palette, size_t pageNumber,
                           size_t pageCount, const std::string &ipText,
                           calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderHomeRhythmPage(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                          const HomeWeatherPalette &palette, size_t pageNumber = 1,
                          size_t pageCount = 1, const std::string &ipText = "IP: --",
                          calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderHomeAtlasPage(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                         const HomeWeatherPalette &palette, size_t pageNumber = 1,
                         size_t pageCount = 1, const std::string &ipText = "IP: --",
                         calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderHomePrintPage(IDrawSurface &surface, const HomeWeatherSnapshot &snapshot,
                         const HomeWeatherPalette &palette, size_t pageNumber = 1,
                         size_t pageCount = 1, const std::string &ipText = "IP: --",
                         calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
