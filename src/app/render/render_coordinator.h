#pragma once

#include <stdint.h>

#include "app/page/page_catalog.h"

struct RenderInputs {
    bool epdBusy = false;
    PageId requestedPage = PageId::WeatherToday;
    PageId currentPage = PageId::WeatherToday;
    uint32_t contentHash = 0;
    uint32_t lastContentHash = 0;
    int64_t nowUtc = 0;
    int64_t lastFullRefreshUtc = 0;
    uint32_t minFullRefreshIntervalSec = 10;
    bool forceRefresh = false;
};

struct RenderDecision {
    bool shouldRender = true;
    bool deferred = false;
    PageId targetPage = PageId::WeatherToday;
};

class RenderCoordinator {
public:
    RenderDecision decide(const RenderInputs &input) const;
};
