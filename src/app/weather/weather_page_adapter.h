#pragma once

#include <stdint.h>

#include <string>

#if defined(UI_LAYOUT_EPD_400x300)
#include "ui/layouts/epd_400x300/render_weather.h"

WeatherPageSnapshot weatherFallbackPageSnapshot(const std::string &city,
                                                const std::string &region,
                                                const std::string &country,
                                                const std::string &tempUnit,
                                                int64_t localEpoch);
#endif
