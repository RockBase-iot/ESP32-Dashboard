#pragma once

#include <stdint.h>

#include "app/input/button_controller.h"
#include "app/page/page_catalog.h"
#include "app/page/page_manager.h"

constexpr int32_t kStoredHomeWeatherPage = -1;

class DisplayPageState {
public:
    static DisplayPageState homeWeather();
    static DisplayPageState managed(PageId page);

    bool isHomeWeather() const { return _homeWeather; }
    PageId managedPage() const { return _page; }

private:
    bool _homeWeather = true;
    PageId _page = PageId::WeatherToday;
};

DisplayPageState sanitizeStoredDisplayPage(int32_t rawPageId, const PageSettings &settings);
DisplayPageState selectStartupDisplayPage(DisplayPageState persistedPage,
                                          const PageSettings &settings,
                                          bool restorePersistedPage);
DisplayPageState selectStartupDisplayPage(DisplayPageState persistedPage,
                                          const PageSettings &settings,
                                          bool restorePersistedPage,
                                          bool forcePersistedPage);
DisplayPageState loadPersistedDisplayPage(const PageSettings &settings);
bool savePersistedDisplayPage(DisplayPageState page);
int32_t storedValueForDisplayPage(DisplayPageState page);
DisplayPageState applyButtonDisplayPageAction(PageManager &manager,
                                              DisplayPageState current,
                                              ButtonAction action);
