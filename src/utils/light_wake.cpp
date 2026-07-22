#include "light_wake.h"

namespace {
bool pinInWakeStatus(uint64_t status, uint8_t pin) {
    return pin != 0xFF && pin < 64 && (status & (1ULL << pin)) != 0;
}
}  // namespace

LightWake classifyLightWake(bool timerFired, bool gpioFired,
                            uint64_t gpioWakeStatus,
                            uint8_t bootPin, uint8_t userPin,
                            bool bootPressedNow, bool userPressedNow) {
    // Buttons first: hardware status, then live pin level as fallback (a fast
    // press/release can leave the status register at zero).
    if (bootPin != 0xFF && (pinInWakeStatus(gpioWakeStatus, bootPin) || bootPressedNow)) {
        return LightWake::BootButton;
    }
    if (userPin != 0xFF && (pinInWakeStatus(gpioWakeStatus, userPin) || userPressedNow)) {
        return LightWake::UserButton;
    }
    if (timerFired) {
        return LightWake::Timeout;
    }
    // gpioFired without an attributable pin, or a spurious wake with no cause.
    (void)gpioFired;
    return LightWake::Other;
}

bool interactiveWindowShouldExit(LightWake wake, uint32_t elapsedMs, uint32_t budgetMs) {
    if (wake == LightWake::Timeout) {
        return true;
    }
    return elapsedMs >= budgetMs;
}
