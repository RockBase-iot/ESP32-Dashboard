#include "focus_clock_controller.h"

namespace {
constexpr uint32_t kFocusRefreshMs = 60UL * 1000UL;
constexpr uint32_t kFocusStopHoldMs = 2000UL;

int64_t totalFocusSessionSeconds(const FocusClockConfig &config) {
    const FocusClockConfig clean = normalizeFocusClockConfig(config);
    return static_cast<int64_t>(clean.focusMinutes + clean.breakMinutes) *
           60LL * clean.sessionCount;
}
}  // namespace

bool applyFocusClockButtonAction(PageId page,
                                 ButtonAction action,
                                 const FocusClockConfig &config,
                                 FocusClockRuntimeState &state,
                                 int64_t nowUtc) {
    if (page != PageId::FocusClock || action != ButtonAction::SyncCurrent) {
        return false;
    }

    if (focusClockSessionIsActive(config, state, nowUtc)) {
        state = stopFocusClockSession();
    } else {
        state = startFocusClockSession(config, nowUtc);
    }
    return true;
}

bool focusClockBlocksButtonAction(PageId page,
                                  ButtonAction action,
                                  const FocusClockConfig &config,
                                  const FocusClockRuntimeState &state,
                                  int64_t nowUtc) {
    if (page != PageId::FocusClock ||
        !focusClockSessionIsActive(config, state, nowUtc)) {
        return false;
    }
    return action == ButtonAction::NextPage ||
           action == ButtonAction::PreviousPage ||
           action == ButtonAction::OpenConfig ||
           action == ButtonAction::RecoveryAp;
}

uint32_t focusClockNextRefreshMs(const FocusClockConfig &config,
                                 const FocusClockRuntimeState &state,
                                 int64_t nowUtc) {
    if (!focusClockSessionIsActive(config, state, nowUtc)) {
        return 0;
    }
    const int64_t remainingSec = state.startedUtc + totalFocusSessionSeconds(config) - nowUtc;
    if (remainingSec <= 0) {
        return 0;
    }
    const uint64_t remainingMs = static_cast<uint64_t>(remainingSec) * 1000ULL;
    return remainingMs < kFocusRefreshMs ? static_cast<uint32_t>(remainingMs) : kFocusRefreshMs;
}

ButtonAction focusClockActionFromLightWake(LightWake wake, uint32_t heldAfterWakeMs) {
    if (wake == LightWake::UserButton && heldAfterWakeMs >= kFocusStopHoldMs) {
        return ButtonAction::SyncCurrent;
    }
    return ButtonAction::None;
}

ButtonAction focusClockActionFromAwakeHold(PageId page,
                                           bool userPressed,
                                           uint32_t heldMs,
                                           bool alreadyHandled) {
    if (page == PageId::FocusClock && userPressed && !alreadyHandled &&
        heldMs >= kFocusStopHoldMs) {
        return ButtonAction::SyncCurrent;
    }
    return ButtonAction::None;
}
