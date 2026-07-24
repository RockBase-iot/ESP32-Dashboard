#include "display_page_state.h"

#include "app/config/nvs_table.h"
#include "app/config/settings.h"

namespace {
bool isEnabledInSettings(PageId page, const PageSettings &settings) {
    return isValidPageId(page) && (settings.enabledMask & pageMask(page)) != 0;
}

PageId firstManagedPage(PageManager &manager) {
    return manager.restoreStoredPage(manager.firstPage());
}
}  // namespace

DisplayPageState DisplayPageState::homeWeather() {
    return DisplayPageState{};
}

DisplayPageState DisplayPageState::managed(PageId page) {
    DisplayPageState state;
    state._homeWeather = false;
    state._page = page;
    return state;
}

DisplayPageState sanitizeStoredDisplayPage(int32_t rawPageId, const PageSettings &settings) {
    const PageSettings clean = sanitizePageSettings(settings);
    if (rawPageId == kStoredHomeWeatherPage) {
        return DisplayPageState::homeWeather();
    }
    if (rawPageId < 0 || rawPageId >= static_cast<int32_t>(kPageCount)) {
        return DisplayPageState::homeWeather();
    }
    const PageId page = static_cast<PageId>(rawPageId);
    if (!isEnabledInSettings(page, clean)) {
        return DisplayPageState::homeWeather();
    }
    return DisplayPageState::managed(page);
}

DisplayPageState selectStartupDisplayPage(DisplayPageState persistedPage,
                                          const PageSettings &settings,
                                          bool restorePersistedPage) {
    (void)settings;
    if (!restorePersistedPage) {
        return DisplayPageState::homeWeather();
    }
    return persistedPage;
}

int32_t storedValueForDisplayPage(DisplayPageState page) {
    if (page.isHomeWeather()) {
        return kStoredHomeWeatherPage;
    }
    return static_cast<int32_t>(page.managedPage());
}

DisplayPageState loadPersistedDisplayPage(const PageSettings &settings) {
    Settings store(NVS_NAMESPACE_WEATHER, false);
    const int32_t rawPageId = store.GetI32(NVS_KEY_CURRENT_PAGE, kStoredHomeWeatherPage);
    return sanitizeStoredDisplayPage(rawPageId, settings);
}

bool savePersistedDisplayPage(DisplayPageState page) {
    Settings store(NVS_NAMESPACE_WEATHER, true);
    store.SetI32(NVS_KEY_CURRENT_PAGE, storedValueForDisplayPage(page));
    return store.Commit();
}

DisplayPageState applyButtonDisplayPageAction(PageManager &manager,
                                              DisplayPageState current,
                                              ButtonAction action) {
    if (action != ButtonAction::NextPage && action != ButtonAction::PreviousPage) {
        return current;
    }

    if (current.isHomeWeather()) {
        firstManagedPage(manager);
        if (action == ButtonAction::PreviousPage && manager.pageCount() > 0) {
            return DisplayPageState::managed(manager.previousManual());
        }
        return DisplayPageState::managed(manager.current());
    }

    manager.restoreStoredPage(current.managedPage());
    const size_t currentNumber = manager.pageNumber(current.managedPage());
    const size_t count = manager.pageCount();
    if ((action == ButtonAction::NextPage && currentNumber >= count) ||
        (action == ButtonAction::PreviousPage && currentNumber <= 1)) {
        firstManagedPage(manager);
        return DisplayPageState::homeWeather();
    }

    return DisplayPageState::managed(action == ButtonAction::NextPage
                                         ? manager.nextManual()
                                         : manager.previousManual());
}
