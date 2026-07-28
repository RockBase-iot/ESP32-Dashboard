#include "render_news.h"

#include <algorithm>
#include <array>

#include "ui/components/calm_grid.h"

namespace {
void drawNewsHeadlineRow(IDrawSurface &surface, const Rect &rect, int index,
                         const NewsItemCell &item) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, kDashboardBlack);
    surface.drawLine(rect.x, static_cast<int16_t>(rect.y + rect.h - 1),
                     static_cast<int16_t>(rect.x + rect.w), static_cast<int16_t>(rect.y + rect.h - 1),
                     kDashboardBlack);
    surface.fillRect(static_cast<int16_t>(rect.x + 12), static_cast<int16_t>(rect.y + 10),
                     6, 12, kDashboardAccent);
    surface.drawText(static_cast<int16_t>(rect.x + 28), static_cast<int16_t>(rect.y + 12),
                     calm_grid::fitText(surface, item.title, static_cast<int16_t>(rect.w - 104), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + rect.w - 10), static_cast<int16_t>(rect.y + 12),
                     calm_grid::fitText(surface, item.source, 48, 1),
                     kDashboardBlack, TextAlign::Right, 1);
    // surface.drawText(static_cast<int16_t>(rect.x + rect.w - 10), static_cast<int16_t>(rect.y + 30),
    //                  calm_grid::fitText(surface, item.detail, 48, 1),
    //                  kDashboardBlack, TextAlign::Right, 1);
    (void)index;
}

void drawNewsHistoryCard(IDrawSurface &surface, const Rect &rect, const NewsItemCell &item) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, kDashboardAccent);
    surface.drawText(static_cast<int16_t>(rect.x + 18), static_cast<int16_t>(rect.y + 13),
                     item.title, kDashboardAccent, TextAlign::Left, 2);
    surface.drawText(static_cast<int16_t>(rect.x + 86), static_cast<int16_t>(rect.y + 7),
                     "Today in History", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 86), static_cast<int16_t>(rect.y + 18),
                     calm_grid::fitText(surface, item.detail, static_cast<int16_t>(rect.w - 100), 1),
                     kDashboardBlack, TextAlign::Left, 1);
}

void renderNewsLayout(IDrawSurface &surface, const NewsPageSnapshot &snapshot,
                      const std::string &headerTitle, size_t pageNumber, size_t pageCount,
                      const std::string &ipText, calm_grid::ChromeContext chrome) {
    calm_grid::drawPrototypePageChrome(surface, headerTitle, calm_grid::PageIconKind::News,
                                       pageNumber, pageCount, chrome.timeText,
                                       ipText.empty() ? chrome.ipText : ipText,
                                       chrome.batteryText);
    surface.drawRect(18, 62, 364, 50, kDashboardBlack);
    surface.drawText(26, 75, "TOP STORIES", kDashboardAccent, TextAlign::Left, 1);
    if (!snapshot.subtitle.empty()) {
        surface.drawText(374, 75, calm_grid::fitText(surface, snapshot.subtitle, 116, 1),
                         kDashboardBlack, TextAlign::Right, 1);
    }
    surface.drawLine(26, 86, 374, 86, kDashboardBlack);
    if (!snapshot.headlines.empty()) {
        surface.drawText(28, 98,
                         calm_grid::fitText(surface, snapshot.headlines.front().title, 340, 1),
                         kDashboardBlack, TextAlign::Left, 1);
        // surface.drawText(28, 120,
        //                  calm_grid::fitText(surface, snapshot.headlines.front().detail, 340, 1),
        //                  kDashboardBlack, TextAlign::Left, 1);
    } else {
        surface.drawText(28, 98, "NO HEADLINES", kDashboardAccent, TextAlign::Left, 1);
        surface.drawText(28, 110, "Check RSS/Atom feeds", kDashboardBlack, TextAlign::Left, 1);
    }

    const std::array<Rect, 3> rows = {{{18, 120, 364, 30},
                                       {18, 158, 364, 30},
                                       {18, 196, 364, 30}}};
    const size_t count = std::min<size_t>(rows.size(), snapshot.headlines.size());
    for (size_t i = 0; i < count; ++i) {
        drawNewsHeadlineRow(surface, rows[i], static_cast<int>(i + 1), snapshot.headlines[i]);
    }
    if (!snapshot.history.empty()) {
        drawNewsHistoryCard(surface, Rect{18, 234, 364, 46}, snapshot.history.front());
    }
}
}  // namespace

NewsPageSnapshot sampleNewsPageSnapshot() {
    NewsPageSnapshot snapshot;
    snapshot.title = "HEADLINES";
    snapshot.subtitle = "RSS  8/12";
    snapshot.headlines = {
        {"Markets open higher after new inflation report", "Reuters", "12m", true},
        {"City approves expanded regional transit plan", "Local", "28m", false},
        {"Space mission completes successful lunar flyby", "Science", "41m", false},
    };
    snapshot.history = {
        {"1969", "Today in History", "Apollo 11 astronauts walked on the Moon.", true},
    };
    return snapshot;
}

void renderHeadlinesPage(IDrawSurface &surface, const NewsPageSnapshot &snapshot,
                         size_t pageNumber, size_t pageCount, const std::string &ipText,
                         calm_grid::ChromeContext chrome) {
    renderNewsLayout(surface, snapshot, "HEADLINES", pageNumber, pageCount, ipText, chrome);
}

void renderTodayInHistoryPage(IDrawSurface &surface, const NewsPageSnapshot &snapshot,
                              size_t pageNumber, size_t pageCount, const std::string &ipText,
                              calm_grid::ChromeContext chrome) {
    renderNewsLayout(surface, snapshot, "TODAY IN HISTORY", pageNumber, pageCount, ipText,
                     chrome);
}
