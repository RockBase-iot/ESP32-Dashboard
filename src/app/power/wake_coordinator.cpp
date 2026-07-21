#include "wake_coordinator.h"

PowerDecision WakeCoordinator::decide(const WakeInputs &input) const {
    PowerDecision decision;
    decision.nextState = input.currentState;
    decision.timerWakeUs = input.deepSleepTimerUs;

    if (input.nightWindow || input.signal == WakeSignal::NightWindow) {
        decision.nextState = PowerState::DeepSleep;
        return decision;
    }

    if (input.currentState == PowerState::Interactive) {
        if (input.signal == WakeSignal::Inactivity &&
            input.secondsSinceActivity >= kInteractiveIdleSeconds) {
            decision.nextState = PowerState::DeepSleep;
        }
        return decision;
    }

    if (input.currentState == PowerState::DeepSleep) {
        if (input.signal == WakeSignal::Boot || input.signal == WakeSignal::Timer) {
            decision.nextState = PowerState::Interactive;
            decision.timerWakeUs = 0;
        } else {
            decision.nextState = PowerState::DeepSleep;
        }
    }

    return decision;
}
