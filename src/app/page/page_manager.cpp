#include "page_manager.h"

#include <algorithm>

#if defined(ARDUINO)
#include <esp_attr.h>
RTC_DATA_ATTR RtcPageState gDashboardRtcPageState;
#endif

bool isValidRotationIntervalMinutes(uint16_t minutes) {
    return minutes == 0 || minutes == 30 || minutes == 60 || minutes == 90 ||
           minutes == 120 || minutes == 150;
}

std::string timeZoneIdForUtcOffset(int utcOffsetHours) {
    switch (utcOffsetHours) {
        case -8:
            return "America/Los_Angeles";
        case -7:
            return "America/Denver";
        case -6:
            return "America/Chicago";
        case -5:
            return "America/New_York";
        case 0:
            return "Etc/UTC";
        case 1:
            return "Europe/Berlin";
        case 8:
            return "Asia/Shanghai";
        case 9:
            return "Asia/Tokyo";
        default:
            return "Etc/GMT";
    }
}

PageSettings defaultPageSettings() {
    PageSettings settings;
    settings.enabledMask = defaultHomePageMask();
    settings.autoRotateMask = defaultAutoRotateMask();
    settings.orderCount = pageCatalog().size();
    const std::array<PageId, kPageCount> defaultOrder = {
        PageId::WeatherToday,
        PageId::Overview,
        PageId::TodayAgenda,
        PageId::WeeklyTimeline,
        PageId::MonthlyOverview,
        PageId::LocalNotes,
        PageId::WeeklyWeather,
        PageId::IndoorClimate,
        PageId::WorldClock,
        PageId::FocusClock,
        PageId::StockInfo,
        PageId::PortfolioSummary,
        PageId::EconomicCalendar,
        PageId::Headlines,
        PageId::TodayInHistory,
        PageId::ImportantMilestones,
    };
    for (size_t i = 0; i < defaultOrder.size() && i < settings.order.size(); ++i) {
        settings.order[i] = defaultOrder[i];
    }
    settings.templateId = PageTemplateId::CalmGrid;
    settings.rotationIntervalMinutes = 0;
    settings.timeZoneId = "Asia/Shanghai";
    return settings;
}

PageSettings sanitizePageSettings(const PageSettings &settings) {
    PageSettings clean = settings;
    clean.configVersion = kDashboardConfigVersion;
    clean.enabledMask &= (1UL << kPageCount) - 1UL;
    clean.autoRotateMask &= clean.enabledMask;
    if (clean.enabledMask == 0) {
        clean.enabledMask = pageMask(PageId::Overview);
    }
    if (!isValidRotationIntervalMinutes(clean.rotationIntervalMinutes)) {
        clean.rotationIntervalMinutes = 0;
    }
    if (static_cast<uint8_t>(clean.templateId) > static_cast<uint8_t>(PageTemplateId::DenseGrid)) {
        clean.templateId = PageTemplateId::CalmGrid;
    }
    if (clean.timeZoneId.empty()) {
        clean.timeZoneId = "Etc/UTC";
    }

    std::array<bool, kPageCount> seen{};
    std::array<PageId, kPageCount> order{};
    size_t count = 0;
    for (size_t i = 0; i < clean.orderCount && i < clean.order.size(); ++i) {
        const PageId id = clean.order[i];
        if (!isValidPageId(id)) {
            continue;
        }
        const uint8_t index = static_cast<uint8_t>(id);
        if (seen[index]) {
            continue;
        }
        seen[index] = true;
        order[count++] = id;
    }
    for (const PageDescriptor &descriptor : pageCatalog()) {
        const uint8_t index = static_cast<uint8_t>(descriptor.id);
        if (!seen[index]) {
            seen[index] = true;
            order[count++] = descriptor.id;
        }
    }
    clean.order = order;
    clean.orderCount = count;
    return clean;
}

PageManager::PageManager(const PageSettings &settings)
    : _settings(sanitizePageSettings(settings)), _current(firstEnabledPage()) {}

size_t PageManager::pageCount() const {
    size_t count = 0;
    for (size_t i = 0; i < _settings.orderCount; ++i) {
        if (isEnabled(_settings.order[i])) {
            ++count;
        }
    }
    return count;
}

size_t PageManager::pageNumber(PageId id) const {
    size_t number = 0;
    for (size_t i = 0; i < _settings.orderCount; ++i) {
        if (!isEnabled(_settings.order[i])) {
            continue;
        }
        ++number;
        if (_settings.order[i] == id) {
            return number;
        }
    }
    return 1;
}

PageId PageManager::firstPage() const {
    return firstEnabledPage();
}

std::vector<PageId> PageManager::rotationQueue() const {
    std::vector<PageId> queue;
    for (size_t i = 0; i < _settings.orderCount; ++i) {
        const PageId id = _settings.order[i];
        if (isEnabled(id) && (_settings.autoRotateMask & pageMask(id)) != 0) {
            queue.push_back(id);
        }
    }
    return queue;
}

PageId PageManager::nextManual() {
    return moveManual(1);
}

PageId PageManager::previousManual() {
    return moveManual(-1);
}

PageId PageManager::nextAuto() {
    const std::vector<PageId> queue = rotationQueue();
    if (queue.empty()) {
        return _current;
    }
    size_t index = _autoCursor % queue.size();
    if (queue.size() > 1 && queue[index] == _current) {
        index = (index + 1) % queue.size();
    }
    _current = queue[index];
    _autoCursor = (index + 1) % queue.size();
    return _current;
}

RtcPageState PageManager::snapshotRtcState(int64_t lastFullRefreshUtc, uint32_t lastContentHash) const {
    RtcPageState state;
    state.currentPage = _current;
    state.autoCursor = _autoCursor;
    state.configVersion = _settings.configVersion;
    state.lastFullRefreshUtc = lastFullRefreshUtc;
    state.lastContentHash = lastContentHash;
    return state;
}

PageId PageManager::restoreRtcState(const RtcPageState &state) {
    const std::vector<PageId> queue = rotationQueue();
    if (state.configVersion != _settings.configVersion || !isEnabled(state.currentPage)) {
        _current = firstEnabledPage();
        _autoCursor = 0;
        return _current;
    }
    _current = state.currentPage;
    _autoCursor = queue.empty() ? 0 : state.autoCursor % queue.size();
    return _current;
}

PageId PageManager::restoreStoredPage(PageId page) {
    _current = isEnabled(page) ? page : firstEnabledPage();
    _autoCursor = 0;
    return _current;
}

bool PageManager::isEnabled(PageId id) const {
    return isValidPageId(id) && (_settings.enabledMask & pageMask(id)) != 0;
}

PageId PageManager::firstEnabledPage() const {
    for (size_t i = 0; i < _settings.orderCount; ++i) {
        if (isEnabled(_settings.order[i])) {
            return _settings.order[i];
        }
    }
    return PageId::Overview;
}

PageId PageManager::moveManual(int direction) {
    if (_settings.orderCount == 0) {
        _current = PageId::Overview;
        return _current;
    }
    size_t currentIndex = 0;
    for (size_t i = 0; i < _settings.orderCount; ++i) {
        if (_settings.order[i] == _current) {
            currentIndex = i;
            break;
        }
    }
    for (size_t step = 1; step <= _settings.orderCount; ++step) {
        const int raw = static_cast<int>(currentIndex) + direction * static_cast<int>(step);
        const size_t index = static_cast<size_t>(
            (raw % static_cast<int>(_settings.orderCount) + static_cast<int>(_settings.orderCount)) %
            static_cast<int>(_settings.orderCount));
        if (isEnabled(_settings.order[index])) {
            _current = _settings.order[index];
            return _current;
        }
    }
    _current = firstEnabledPage();
    return _current;
}

PageId applyButtonPageAction(PageManager &manager, ButtonAction action) {
    if (action == ButtonAction::NextPage) {
        return manager.nextManual();
    }
    if (action == ButtonAction::PreviousPage) {
        return manager.previousManual();
    }
    return manager.current();
}
