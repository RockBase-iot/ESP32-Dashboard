#include "render_time.h"

#include <algorithm>
#include <array>

#include "ui/components/calm_grid.h"

namespace {
void drawClockHeader(IDrawSurface &surface, const std::string &title, const std::string &rightText) {
    calm_grid::drawPageHeader(surface, title, rightText, "", 74);
}

void drawWorldClockHeader(IDrawSurface &surface, const std::string &dateText,
                          const std::string &pageTitle) {
    surface.fillScreen(kDashboardWhite);
    surface.drawRect(10, 12, 12, 12, kDashboardBlack);
    surface.drawLine(12, 17, 20, 17, kDashboardBlack);
    surface.drawLine(14, 15, 14, 19, kDashboardBlack);
    surface.drawText(28, 14, calm_grid::fitText(surface, dateText, 180, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(28, 31, calm_grid::fitText(surface, pageTitle, 168, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawLine(18, 45, static_cast<int16_t>(surface.width() - 18), 45, kDashboardBlack);
    surface.drawLine(18, 46, 92, 46, kDashboardAccent);
}

void drawClockFooter(IDrawSurface &surface, const std::string &leftText, const std::string &rightText) {
    if (!leftText.empty()) {
        surface.drawText(18, 286, calm_grid::fitText(surface, leftText, 176, 1),
                         kDashboardBlack, TextAlign::Left, 1);
    }
    if (!rightText.empty()) {
        surface.drawText(382, 286, calm_grid::fitText(surface, rightText, 176, 1),
                         kDashboardBlack, TextAlign::Right, 1);
    }
}

std::string clockDisplayLabel(const std::string &label) {
    if (label == "New York") {
        return "NEW YORK";
    }
    if (label == "London") {
        return "LONDON";
    }
    if (label == "Berlin") {
        return "BERLIN";
    }
    if (label == "Shanghai") {
        return "SHANGHAI";
    }
    return label;
}

std::string clockZoneStatus(const std::string &label) {
    if (label == "New York") {
        return "EDT - WORKING";
    }
    if (label == "London") {
        return "BST - WORKING";
    }
    if (label == "Berlin") {
        return "CEST - WORKING";
    }
    if (label == "Shanghai") {
        return "CST - EVENING";
    }
    return "LOCAL - READY";
}

std::string clockHeaderMetadata(const WorldClockPageSnapshot &snapshot,
                                const std::string &pageTitle) {
    const std::string pageIndicator = snapshot.pageIndicator.empty()
        ? "4/12"
        : snapshot.pageIndicator;
    if (snapshot.subtitle.empty() || snapshot.subtitle == pageTitle) {
        return pageIndicator;
    }
    return snapshot.subtitle + "   " + pageIndicator;
}

void drawWorldClockCard(IDrawSurface &surface, const Rect &rect, const WorldClockSlot &slot,
                        bool accentTime) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + 14),
                     calm_grid::fitText(surface, clockDisplayLabel(slot.label),
                                        static_cast<int16_t>(rect.w - 20), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + 38),
                     slot.timeText, accentTime ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 3);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + rect.h - 10),
                     calm_grid::fitText(surface, clockZoneStatus(slot.label),
                                        static_cast<int16_t>(rect.w - 20), 1),
                     accentTime ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 1);
}
}  // namespace

WorldClockPageSnapshot sampleWorldClockPageSnapshot() {
    return worldClockPageSnapshotAt(1784551320LL, "Asia/Shanghai", 4, 12);
}

WorldClockPageSnapshot worldClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount) {
    WorldClockPageSnapshot snapshot;
    snapshot.title = "WORLD CLOCK";
    snapshot.subtitle = formatWorldClockDateLabel(nowUtc, timezoneId);
    snapshot.pageIndicator = std::to_string(pageNumber) + "/" + std::to_string(pageCount);
    snapshot.model = buildWorldClock(sampleWorldClockConfig(), nowUtc);
    snapshot.model.focusLabel = "FOCUS CLOCK";
    snapshot.model.focusText = "Next sync at 14:30";
    return snapshot;
}

void renderWorldClockPage(IDrawSurface &surface, const WorldClockPageSnapshot &snapshot) {
    const std::string pageTitle = snapshot.title.empty() ? "WORLD CLOCK" : snapshot.title;
    drawWorldClockHeader(surface, clockHeaderMetadata(snapshot, pageTitle), pageTitle);
    const std::array<Rect, 4> cards = {{
        {18, 54, 176, 82},
        {206, 54, 176, 82},
        {18, 144, 176, 82},
        {206, 144, 176, 82},
    }};
    const size_t count = std::min<size_t>(cards.size(), snapshot.model.clocks.size());
    for (size_t i = 0; i < count; ++i) {
        drawWorldClockCard(surface, cards[i], snapshot.model.clocks[i],
                           snapshot.model.clocks[i].label == "London");
    }
    drawClockFooter(surface, "12/24 hour configurable", "");
}

void renderFocusClockPage(IDrawSurface &surface, const WorldClockPageSnapshot &snapshot) {
    const std::string pageTitle = "FOCUS CLOCK";
    drawClockHeader(surface, pageTitle, clockHeaderMetadata(snapshot, pageTitle));
    surface.drawRect(18, 62, 364, 74, kDashboardBlack);
    surface.drawText(28, 86,
                     snapshot.model.focusLabel.empty() ? "FOCUS CLOCK" : snapshot.model.focusLabel,
                     kDashboardAccent, TextAlign::Left, 2);
    surface.drawText(28, 112, snapshot.model.focusText, kDashboardBlack, TextAlign::Left, 1);

    const size_t count = std::min<size_t>(2, snapshot.model.clocks.size());
    for (size_t i = 0; i < count; ++i) {
        const Rect rect{static_cast<int16_t>(18 + 188 * static_cast<int16_t>(i)), 156, 176, 74};
        drawWorldClockCard(surface, rect, snapshot.model.clocks[i], i == 1);
    }
    drawClockFooter(surface, "Pinned city view", "Updated by SNTP");
}
