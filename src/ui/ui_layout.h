#pragma once

// Compile-time layout routing — selected by build_flags in platformio.ini.
// App layer includes only this header; concrete page types are resolved here.

#if defined(UI_LAYOUT_EPD_400x300)
    #include "layouts/epd_400x300/page_weather.h"
    #include "layouts/epd_400x300/page_error.h"
    #include "layouts/epd_400x300/page_loading.h"
    using PageWeather = PageWeather400x300;
    using PageError   = PageError400x300;
    using PageLoading = PageLoading400x300;
#elif defined(UI_LAYOUT_EPD_800x480)
    #include "layouts/epd_800x480/page_weather.h"
    #include "layouts/epd_800x480/page_error.h"
    #include "layouts/epd_800x480/page_loading.h"
    using PageWeather = PageWeather800x480;
    using PageError   = PageError800x480;
    using PageLoading = PageLoading800x480;
#else
    #error "No UI layout defined. Add -DUI_LAYOUT_EPD_400x300 (or similar) to build_flags."
#endif
