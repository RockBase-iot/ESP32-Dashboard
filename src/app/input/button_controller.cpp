#include "button_controller.h"

ButtonAction ButtonController::update(ButtonId button, bool pressed, uint32_t nowMs) {
    ButtonState &state = stateFor(button);

    if (pressed == state.stablePressed) {
        state.hasCandidate = false;
        return ButtonAction::None;
    }

    if (!state.hasCandidate || state.candidatePressed != pressed) {
        state.hasCandidate = true;
        state.candidatePressed = pressed;
        state.candidateSinceMs = nowMs;
        return ButtonAction::None;
    }

    if (nowMs - state.candidateSinceMs < kDebounceMs) {
        return ButtonAction::None;
    }

    state.hasCandidate = false;
    state.stablePressed = pressed;
    if (pressed) {
        state.pressedSinceMs = nowMs;
        return ButtonAction::None;
    }

    const uint32_t heldMs = nowMs - state.pressedSinceMs;
    return actionForRelease(button, heldMs);
}

ButtonAction ButtonController::actionForRelease(ButtonId button, uint32_t heldMs) const {
    if (button == ButtonId::Boot) {
        if (heldMs >= kRecoveryPressMs) {
            return ButtonAction::RecoveryAp;
        }
        if (heldMs >= kLongPressMs) {
            return ButtonAction::OpenConfig;
        }
        return ButtonAction::NextPage;
    }

    if (heldMs >= kLongPressMs) {
        return ButtonAction::SyncCurrent;
    }
    return ButtonAction::PreviousPage;
}

ButtonController::ButtonState &ButtonController::stateFor(ButtonId button) {
    return button == ButtonId::Boot ? _boot : _user;
}
