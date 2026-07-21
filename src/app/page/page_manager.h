#pragma once

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <string>
#include <vector>

#include "app/input/button_controller.h"
#include "app/page/page_catalog.h"

enum class PageTemplateId : uint8_t {
    CalmGrid,
    DenseGrid,
};

struct PageSettings {
    uint32_t configVersion = kDashboardConfigVersion;
    uint32_t enabledMask = 0;
    uint32_t autoRotateMask = 0;
    std::array<PageId, kPageCount> order{};
    size_t orderCount = 0;
    PageTemplateId templateId = PageTemplateId::CalmGrid;
    uint16_t rotationIntervalMinutes = 0;
    std::string timeZoneId;
};

struct RtcPageState {
    PageId currentPage = PageId::Overview;
    size_t autoCursor = 0;
    uint32_t configVersion = 0;
    int64_t lastFullRefreshUtc = 0;
    uint32_t lastContentHash = 0;
};

bool isValidRotationIntervalMinutes(uint16_t minutes);
std::string timeZoneIdForUtcOffset(int utcOffsetHours);
PageSettings defaultPageSettings();
PageSettings sanitizePageSettings(const PageSettings &settings);

class PageManager {
public:
    explicit PageManager(const PageSettings &settings);

    PageId current() const { return _current; }
    size_t autoCursor() const { return _autoCursor; }
    size_t pageCount() const;
    size_t pageNumber(PageId id) const;
    std::vector<PageId> rotationQueue() const;

    PageId nextManual();
    PageId previousManual();
    PageId nextAuto();
    RtcPageState snapshotRtcState(int64_t lastFullRefreshUtc, uint32_t lastContentHash = 0) const;
    PageId restoreRtcState(const RtcPageState &state);
    PageId restoreStoredPage(PageId page);

private:
    bool isEnabled(PageId id) const;
    PageId firstEnabledPage() const;
    PageId moveManual(int direction);

    PageSettings _settings;
    PageId _current = PageId::Overview;
    size_t _autoCursor = 0;
};

PageId applyButtonPageAction(PageManager &manager, ButtonAction action);

#if defined(ARDUINO)
extern RtcPageState gDashboardRtcPageState;
#endif
