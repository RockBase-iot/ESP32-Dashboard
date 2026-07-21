#pragma once

#include <stdint.h>

enum class PowerState : uint8_t {
    Interactive = 0,
    DeepSleep,
};

enum class WakeSignal : uint8_t {
    None = 0,
    Boot,
    User,
    Timer,
    Inactivity,
    NightWindow,
};

struct WakeInputs {
    PowerState currentState = PowerState::Interactive;
    WakeSignal signal = WakeSignal::None;
    uint32_t secondsSinceActivity = 0;
    bool nightWindow = false;
    uint64_t deepSleepTimerUs = 0;
};

struct PowerDecision {
    PowerState nextState = PowerState::Interactive;
    uint64_t timerWakeUs = 0;
};

class WakeCoordinator {
public:
    PowerDecision decide(const WakeInputs &input) const;

private:
    static constexpr uint32_t kInteractiveIdleSeconds = 30;
};
