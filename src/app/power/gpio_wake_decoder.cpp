#include "gpio_wake_decoder.h"

namespace {
bool pinInWakeStatus(uint64_t status, uint8_t pin) {
    return pin < 64 && (status & (1ULL << pin)) != 0;
}
}  // namespace

WakeSignal decodeGpioWakeSignal(uint64_t gpioWakeStatus,
                                uint8_t bootPin,
                                uint8_t userPin,
                                bool bootPressedNow,
                                bool userPressedNow) {
    if (pinInWakeStatus(gpioWakeStatus, bootPin)) {
        return WakeSignal::Boot;
    }
    if (pinInWakeStatus(gpioWakeStatus, userPin)) {
        return WakeSignal::User;
    }
    if (bootPressedNow) {
        return WakeSignal::Boot;
    }
    if (userPressedNow) {
        return WakeSignal::User;
    }
    return WakeSignal::None;
}
