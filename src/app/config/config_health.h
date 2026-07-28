#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "app/config/app_config.h"
#include "app/page/page_manager.h"

enum class PageReadinessState : uint8_t {
    Ready,
    Required,
    Optional,
    Error,
};

struct SourceConfigSummary {
    uint8_t configuredCalendarSources = 0;
    uint8_t enabledCalendarSources = 0;
    uint8_t newsFeeds = 0;
    uint8_t stockSymbols = 0;
    uint8_t portfolioPositions = 0;
    uint8_t economicFeeds = 0;
    uint8_t worldClockZones = 0;
    bool indoorEnabled = false;
    bool focusConfigured = false;
    uint16_t focusMinutes = 25;
    uint16_t focusBreakMinutes = 5;
    uint8_t focusSessionCount = 4;
};

struct PageReadiness {
    PageId page = PageId::WeatherToday;
    bool enabled = false;
    PageReadinessState state = PageReadinessState::Ready;
    std::string dependency;
    std::string message;
};

struct ConfigHealth {
    std::vector<PageReadiness> pages;
    size_t enabledManagedPages = 0;
    size_t totalDisplayPages = 1;  // Page 0 legacy weather home is always on.
    bool allEnabledPagesReady = true;
    size_t requiredCount = 0;
    size_t optionalCount = 0;
    size_t errorCount = 0;
};

const char *pageReadinessStateName(PageReadinessState state);
SourceConfigSummary sourceSummaryFromConfig(const AppConfig &cfg);
ConfigHealth buildConfigHealth(const AppConfig &cfg, const PageSettings &settings,
                               const SourceConfigSummary &sources);
const PageReadiness *findPageReadiness(const ConfigHealth &health, PageId page);
