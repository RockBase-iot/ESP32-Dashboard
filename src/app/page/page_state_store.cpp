#include "page_state_store.h"

#include "app/config/nvs_table.h"
#include "app/config/settings.h"

namespace {
constexpr int32_t kNoStoredPage = -1;

bool isEnabledInSettings(PageId page, const PageSettings &settings) {
    return isValidPageId(page) && (settings.enabledMask & pageMask(page)) != 0;
}

PageId fallbackPage(const PageSettings &settings) {
    const PageSettings clean = sanitizePageSettings(settings);
    if (isEnabledInSettings(PageId::WeatherToday, clean)) {
        return PageId::WeatherToday;
    }
    PageManager manager(clean);
    return manager.current();
}
}  // namespace

PageId sanitizeStoredPageId(int32_t rawPageId, const PageSettings &settings) {
    const PageSettings clean = sanitizePageSettings(settings);
    if (rawPageId < 0 || rawPageId >= static_cast<int32_t>(kPageCount)) {
        return fallbackPage(clean);
    }
    const PageId page = static_cast<PageId>(rawPageId);
    if (!isEnabledInSettings(page, clean)) {
        return fallbackPage(clean);
    }
    return page;
}

PageId loadPersistedCurrentPage(const PageSettings &settings) {
    Settings store(NVS_NAMESPACE_WEATHER, false);
    const int32_t rawPageId = store.GetI32(NVS_KEY_CURRENT_PAGE, kNoStoredPage);
    return sanitizeStoredPageId(rawPageId, settings);
}

bool savePersistedCurrentPage(PageId page) {
    if (!isValidPageId(page)) {
        return false;
    }
    Settings store(NVS_NAMESPACE_WEATHER, true);
    store.SetI32(NVS_KEY_CURRENT_PAGE, static_cast<int32_t>(page));
    return store.Commit();
}
