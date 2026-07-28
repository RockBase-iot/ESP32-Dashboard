#include "render_time.h"

#include <algorithm>
#include <array>
#include <cctype>

#include "ui/components/calm_grid.h"

namespace {
std::string clockDisplayLabel(const std::string &label) {
    std::string out = label;
    for (char &ch : out) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return out;
}

void drawWorldClockCard(IDrawSurface &surface, const Rect &rect, const WorldClockSlot &slot,
                        bool accentTime) {
    surface.drawRect(rect.x, rect.y, rect.w, rect.h, kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + 14),
                     calm_grid::fitText(surface, clockDisplayLabel(slot.label),
                                        static_cast<int16_t>(rect.w - 20), 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + 34),
                     slot.timeText, accentTime ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 3);
    surface.drawText(static_cast<int16_t>(rect.x + 8), static_cast<int16_t>(rect.y + rect.h - 10),
                     calm_grid::fitText(surface,
                                        slot.statusText.empty() ? slot.timezoneId : slot.statusText,
                                        static_cast<int16_t>(rect.w - 20), 1),
                     accentTime ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Left, 1);
}
}  // namespace

WorldClockPageSnapshot sampleWorldClockPageSnapshot() {
    return worldClockPageSnapshotAt(1784551320LL, "Asia/Shanghai", 4, 12);
}

FocusClockPageSnapshot sampleFocusClockPageSnapshot() {
    const FocusClockConfig config = defaultFocusClockConfig();
    const FocusClockRuntimeState state = startFocusClockSession(config, 1784551320LL);
    return focusClockPageSnapshotAt(1784551320LL, "Asia/Shanghai", 10, 16, config, state);
}

WorldClockPageSnapshot worldClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount) {
    return worldClockPageSnapshotAt(nowUtc, timezoneId, pageNumber, pageCount,
                                    sampleWorldClockConfig());
}

WorldClockPageSnapshot worldClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount,
                                                const WorldClockConfig &config) {
    WorldClockPageSnapshot snapshot;
    snapshot.title = "WORLD CLOCK";
    snapshot.subtitle = formatWorldClockChromeTimeLabel(nowUtc, timezoneId);
    snapshot.pageIndicator = std::to_string(pageNumber) + "/" + std::to_string(pageCount);
    snapshot.model = buildWorldClock(config, nowUtc);
    snapshot.model.focusLabel = config.focusLabel.empty() ? "FOCUS CLOCK" : config.focusLabel;
    snapshot.model.focusText = config.focusText.empty() ? "Next sync at 14:30" : config.focusText;
    return snapshot;
}

void renderWorldClockPage(IDrawSurface &surface, const WorldClockPageSnapshot &snapshot,
                          size_t pageNumber, size_t pageCount, const std::string &ipText,
                          calm_grid::ChromeContext chrome) {
    const std::string pageTitle = snapshot.title.empty() ? "WORLD CLOCK" : snapshot.title;
    calm_grid::drawPrototypePageChrome(surface, pageTitle, calm_grid::PageIconKind::Clock,
                                       pageNumber, pageCount,
                                       snapshot.subtitle.empty() ? chrome.timeText : snapshot.subtitle,
                                       ipText.empty() ? chrome.ipText : ipText,
                                       chrome.batteryText);
    const std::array<Rect, 4> cards = {{
        {18, 64, 176, 78},
        {206, 64, 176, 78},
        {18, 154, 176, 78},
        {206, 154, 176, 78},
    }};
    const size_t count = std::min<size_t>(cards.size(), snapshot.model.clocks.size());
    for (size_t i = 0; i < count; ++i) {
        drawWorldClockCard(surface, cards[i], snapshot.model.clocks[i], i == 0);
    }
}

void renderFocusClockPage(IDrawSurface &surface, const FocusClockPageSnapshot &snapshot,
                          size_t pageNumber, size_t pageCount, const std::string &ipText,
                          calm_grid::ChromeContext chrome) {
    const std::string pageTitle = "FOCUS CLOCK";
    calm_grid::drawPrototypePageChrome(surface, pageTitle, calm_grid::PageIconKind::Clock,
                                       pageNumber, pageCount,
                                       snapshot.subtitle.empty() ? chrome.timeText : snapshot.subtitle,
                                       ipText.empty() ? chrome.ipText : ipText,
                                       chrome.batteryText);

    const uint16_t countdownColor = snapshot.active ? kDashboardAccent : kDashboardBlack;
    surface.drawText(200, 72, snapshot.countdownText, countdownColor, TextAlign::Center, 5);
    surface.drawText(200, 132, snapshot.statusText, countdownColor, TextAlign::Center, 1);

    const Rect statusPanel{18, 146, 176, 46};
    const Rect durationPanel{206, 146, 176, 46};
    surface.drawRect(statusPanel.x, statusPanel.y, statusPanel.w, statusPanel.h, kDashboardBlack);
    surface.drawRect(durationPanel.x, durationPanel.y, durationPanel.w, durationPanel.h,
                     kDashboardBlack);
    surface.drawText(28, 156, "SESSION", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(28, 178, snapshot.cycleText, kDashboardAccent, TextAlign::Left, 1);
    surface.drawText(216, 156, snapshot.focusDurationText, kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(216, 178, snapshot.breakDurationText, kDashboardBlack, TextAlign::Left, 1);

    const Rect schedulePanel{18, 204, 364, 62};
    surface.drawRect(schedulePanel.x, schedulePanel.y, schedulePanel.w, schedulePanel.h,
                     kDashboardBlack);
    surface.drawText(30, 216, "NEXT BREAK", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(30, 234, snapshot.nextBreakText, kDashboardAccent, TextAlign::Left, 2);
    surface.drawText(210, 216, "END TIME", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(210, 234, snapshot.endTimeText, kDashboardBlack, TextAlign::Left, 2);
    surface.drawText(200, 254, snapshot.controlText, kDashboardBlack, TextAlign::Center, 1);
}
