#include "config_health.h"

namespace {
bool hasText(const String &value) {
    return value.length() > 0;
}

bool hasWeatherLocation(const AppConfig &cfg) {
    return hasText(cfg.lat) && hasText(cfg.lon) && hasText(cfg.city);
}

uint8_t countCsvItems(const String &csv) {
    uint8_t count = 0;
    bool inToken = false;
    for (int i = 0; i < csv.length(); ++i) {
        const char c = csv.charAt(i);
        if (c == ',' || c == '\n' || c == '\r') {
            if (inToken && count < 255) {
                ++count;
            }
            inToken = false;
            continue;
        }
        if (c != ' ' && c != '\t') {
            inToken = true;
        }
    }
    if (inToken && count < 255) {
        ++count;
    }
    return count;
}

bool tokenLooksLikeFeedUrl(const String &token) {
    return token.startsWith("https://") || token.startsWith("webcal://");
}

uint8_t countFeedUrlItems(const String &csv) {
    uint8_t count = 0;
    String token;
    for (int i = 0; i <= csv.length(); ++i) {
        const char c = i < csv.length() ? csv.charAt(i) : ',';
        if (c == ',' || c == '\n' || c == '\r') {
            token.trim();
            if (tokenLooksLikeFeedUrl(token) && count < 255) {
                ++count;
            }
            token = "";
        } else {
            token += c;
        }
    }
    return count;
}

PageReadiness ready(PageId page, bool enabled, const char *dependency, const char *message) {
    return PageReadiness{page, enabled, PageReadinessState::Ready, dependency, message};
}

PageReadiness required(PageId page, bool enabled, const char *dependency, const char *message) {
    return PageReadiness{page, enabled, PageReadinessState::Required, dependency, message};
}

PageReadiness optional(PageId page, bool enabled, const char *dependency, const char *message) {
    return PageReadiness{page, enabled, PageReadinessState::Optional, dependency, message};
}

bool pageEnabled(PageId page, const PageSettings &settings) {
    return (settings.enabledMask & pageMask(page)) != 0;
}

PageReadiness evaluatePage(PageId page, bool enabled, const AppConfig &cfg,
                           const SourceConfigSummary &sources) {
    const bool weatherReady = hasWeatherLocation(cfg);
    switch (page) {
        case PageId::WeatherToday:
        case PageId::WeeklyWeather:
            return weatherReady
                ? ready(page, enabled, "weather", "Weather location is configured")
                : required(page, enabled, "weather", "Set city, latitude, and longitude");

        case PageId::Overview:
            if (!weatherReady) {
                return required(page, enabled, "weather", "Overview needs a weather location");
            }
            if (sources.enabledCalendarSources == 0) {
                return optional(page, enabled, "calendar", "Calendar is optional for Overview");
            }
            return ready(page, enabled, "weather,calendar", "Overview dependencies are configured");

        case PageId::TodayAgenda:
        case PageId::WeeklyTimeline:
        case PageId::MonthlyOverview:
        case PageId::ImportantMilestones:
            return sources.enabledCalendarSources > 0
                ? ready(page, enabled, "calendar", "Calendar source is configured")
                : required(page, enabled, "calendar", "Add at least one enabled calendar source");

        case PageId::LocalNotes:
            return ready(page, enabled, "notes", "Local notes are available");

        case PageId::IndoorClimate:
            return sources.indoorEnabled
                ? ready(page, enabled, "indoor", "Indoor sensor is enabled")
                : optional(page, enabled, "indoor", "Enable the indoor sensor source when available");

        case PageId::WorldClock:
            return ready(page, enabled, "time", "Device time and default cities are available");

        case PageId::FocusClock:
            return ready(page, enabled, "focus", "Focus clock has local defaults");

        case PageId::StockInfo:
            return sources.stockSymbols > 0
                ? ready(page, enabled, "finance", "Watchlist symbols are configured")
                : required(page, enabled, "finance", "Add at least one watchlist symbol");

        case PageId::PortfolioSummary:
            return (sources.portfolioPositions > 0 || sources.stockSymbols > 0)
                ? ready(page, enabled, "portfolio", "Portfolio or watchlist data is configured")
                : required(page, enabled, "portfolio", "Add a position or use a watchlist");

        case PageId::EconomicCalendar:
            return sources.economicFeeds > 0
                ? ready(page, enabled, "economic", "Economic calendar source is configured")
                : required(page, enabled, "economic", "Choose an economic calendar source");

        case PageId::Headlines:
            return sources.newsFeeds > 0
                ? ready(page, enabled, "news", "News feed is configured")
                : required(page, enabled, "news", "Add or enable at least one news feed");

        case PageId::TodayInHistory:
            return ready(page, enabled, "history", "Built-in history source is available");
    }
    return PageReadiness{page, enabled, PageReadinessState::Error, "page", "Unknown page"};
}
}  // namespace

const char *pageReadinessStateName(PageReadinessState state) {
    switch (state) {
        case PageReadinessState::Ready:
            return "Ready";
        case PageReadinessState::Required:
            return "Required";
        case PageReadinessState::Optional:
            return "Optional";
        case PageReadinessState::Error:
            return "Error";
    }
    return "Error";
}

SourceConfigSummary sourceSummaryFromConfig(const AppConfig &cfg) {
    SourceConfigSummary summary;
    summary.newsFeeds = countCsvItems(cfg.newsFeeds);
    summary.stockSymbols = countCsvItems(cfg.stockSymbols);
    summary.portfolioPositions = countCsvItems(cfg.portfolioPositions);
    summary.economicFeeds = countFeedUrlItems(cfg.economicFeeds);
    summary.worldClockZones = countCsvItems(cfg.worldClockZones);
    summary.indoorEnabled = cfg.indoorSensorEnabled;
    summary.focusConfigured = cfg.focusLabel.length() > 0;
    summary.focusMinutes = cfg.focusMinutes;
    summary.focusBreakMinutes = cfg.focusBreakMinutes;
    summary.focusSessionCount = cfg.focusSessionCount;
    return summary;
}

ConfigHealth buildConfigHealth(const AppConfig &cfg, const PageSettings &settings,
                               const SourceConfigSummary &sources) {
    ConfigHealth health;
    const PageSettings clean = sanitizePageSettings(settings);

    for (const PageDescriptor &descriptor : pageCatalog()) {
        const bool enabled = pageEnabled(descriptor.id, clean);
        if (enabled) {
            ++health.enabledManagedPages;
        }
        PageReadiness readiness = evaluatePage(descriptor.id, enabled, cfg, sources);
        if (enabled) {
            if (readiness.state == PageReadinessState::Required) {
                ++health.requiredCount;
                health.allEnabledPagesReady = false;
            } else if (readiness.state == PageReadinessState::Optional) {
                ++health.optionalCount;
            } else if (readiness.state == PageReadinessState::Error) {
                ++health.errorCount;
                health.allEnabledPagesReady = false;
            }
        }
        health.pages.push_back(readiness);
    }
    health.totalDisplayPages = 1 + health.enabledManagedPages;
    return health;
}

const PageReadiness *findPageReadiness(const ConfigHealth &health, PageId page) {
    for (const PageReadiness &readiness : health.pages) {
        if (readiness.page == page) {
            return &readiness;
        }
    }
    return nullptr;
}
