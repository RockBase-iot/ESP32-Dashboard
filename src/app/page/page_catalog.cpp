#include "page_catalog.h"

namespace {
const std::vector<PageDescriptor> kCatalog = {
    {PageId::Overview, "Overview", PageCategory::Calendar, PagePriority::P0,
     {"calendar", "weather"}, true, true},
    {PageId::TodayAgenda, "Today Agenda", PageCategory::Calendar, PagePriority::P0,
     {"calendar"}, true, true},
    {PageId::WeeklyTimeline, "Weekly Timeline", PageCategory::Calendar, PagePriority::P0,
     {"calendar"}, true, true},
    {PageId::MonthlyOverview, "Monthly Overview", PageCategory::Calendar, PagePriority::P1,
     {"calendar"}, true, true},
    {PageId::LocalNotes, "Local Notes", PageCategory::Notes, PagePriority::P1,
     {}, true, true},
    {PageId::WeatherToday, "Weather Today", PageCategory::Weather, PagePriority::P0,
     {"weather"}, true, true},
    {PageId::WeeklyWeather, "Weekly Weather", PageCategory::Weather, PagePriority::P1,
     {"weather"}, true, true},
    {PageId::IndoorClimate, "Indoor Climate", PageCategory::Weather, PagePriority::P1,
     {"indoor"}, true, false},
    {PageId::WorldClock, "World Clock", PageCategory::Time, PagePriority::P1,
     {}, true, true},
    {PageId::FocusClock, "Focus Clock", PageCategory::Time, PagePriority::P2,
     {}, false, true},
    {PageId::StockInfo, "Stock Info", PageCategory::Finance, PagePriority::P2,
     {"finance"}, false, true},
    {PageId::PortfolioSummary, "Portfolio Summary", PageCategory::Finance, PagePriority::P2,
     {"finance"}, false, true},
    {PageId::EconomicCalendar, "Economic Calendar", PageCategory::Finance, PagePriority::P2,
     {"economic"}, false, true},
    {PageId::Headlines, "Headlines", PageCategory::News, PagePriority::P1,
     {"news"}, true, true},
    {PageId::TodayInHistory, "Today in History", PageCategory::News, PagePriority::P2,
     {"history"}, true, false},
    {PageId::ImportantMilestones, "Important Milestones", PageCategory::Calendar, PagePriority::P2,
     {"calendar"}, true, true},
};
}  // namespace

bool isValidPageId(PageId id) {
    return static_cast<uint8_t>(id) < kPageCount;
}

uint32_t pageMask(PageId id) {
    if (!isValidPageId(id)) {
        return 0;
    }
    return 1UL << static_cast<uint8_t>(id);
}

const std::vector<PageDescriptor> &pageCatalog() {
    return kCatalog;
}

const PageDescriptor *findPage(PageId id) {
    for (const PageDescriptor &descriptor : kCatalog) {
        if (descriptor.id == id) {
            return &descriptor;
        }
    }
    return nullptr;
}

uint32_t defaultHomePageMask() {
    uint32_t mask = 0;
    for (const PageDescriptor &descriptor : kCatalog) {
        if (descriptor.homeDefault) {
            mask |= pageMask(descriptor.id);
        }
    }
    return mask;
}

uint32_t defaultOfficePageMask() {
    uint32_t mask = 0;
    for (const PageDescriptor &descriptor : kCatalog) {
        if (descriptor.officeDefault) {
            mask |= pageMask(descriptor.id);
        }
    }
    return mask;
}

uint32_t defaultAutoRotateMask() {
    return pageMask(PageId::Overview) | pageMask(PageId::TodayAgenda) |
           pageMask(PageId::WeatherToday);
}
