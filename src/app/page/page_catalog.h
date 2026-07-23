#pragma once

#include <stdint.h>

#include <string>
#include <vector>

enum class PageId : uint8_t {
    Overview,
    TodayAgenda,
    WeeklyTimeline,
    MonthlyOverview,
    LocalNotes,
    WeatherToday,
    WeeklyWeather,
    IndoorClimate,
    WorldClock,
    FocusClock,
    StockInfo,
    PortfolioSummary,
    EconomicCalendar,
    Headlines,
    TodayInHistory,
    ImportantMilestones,
};

enum class PageCategory : uint8_t {
    Calendar,
    Weather,
    Time,
    Finance,
    News,
    Notes,
};

enum class PagePriority : uint8_t {
    P0,
    P1,
    P2,
};

struct PageDescriptor {
    PageId id;
    const char *name;
    PageCategory category;
    PagePriority priority;
    std::vector<std::string> requiredProviders;
    bool homeDefault;
    bool officeDefault;
};

constexpr uint8_t kPageCount = 16;
constexpr uint32_t kDashboardConfigVersion = 4;

bool isValidPageId(PageId id);
uint32_t pageMask(PageId id);
const std::vector<PageDescriptor> &pageCatalog();
const PageDescriptor *findPage(PageId id);
uint32_t defaultHomePageMask();
uint32_t defaultOfficePageMask();
uint32_t defaultAutoRotateMask();
