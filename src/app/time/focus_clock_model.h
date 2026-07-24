#pragma once

#include <stdint.h>

#include <string>

enum class FocusClockPhase : uint8_t {
    Ready = 0,
    Focus,
    Break,
    Complete,
};

struct FocusClockConfig {
    std::string label = "Focus";
    uint16_t focusMinutes = 25;
    uint16_t breakMinutes = 5;
    uint8_t sessionCount = 4;
    bool autoStartBreak = false;
};

struct FocusClockRuntimeState {
    bool active = false;
    int64_t startedUtc = 0;
};

struct FocusClockPageSnapshot {
    std::string title;
    std::string subtitle;
    std::string pageIndicator;
    std::string countdownText;
    std::string statusText;
    std::string cycleText;
    std::string focusDurationText;
    std::string breakDurationText;
    std::string nextBreakText;
    std::string endTimeText;
    std::string controlText;
    bool active = false;
    bool inBreak = false;
};

FocusClockConfig normalizeFocusClockConfig(FocusClockConfig config);
FocusClockConfig defaultFocusClockConfig();
FocusClockRuntimeState startFocusClockSession(const FocusClockConfig &config, int64_t nowUtc);
FocusClockRuntimeState stopFocusClockSession();
bool focusClockSessionIsActive(const FocusClockConfig &config,
                               const FocusClockRuntimeState &state,
                               int64_t nowUtc);
FocusClockPageSnapshot focusClockPageSnapshotAt(int64_t nowUtc,
                                                const std::string &timezoneId,
                                                size_t pageNumber,
                                                size_t pageCount,
                                                const FocusClockConfig &config,
                                                const FocusClockRuntimeState &state,
                                                bool twentyFourHour = true);
