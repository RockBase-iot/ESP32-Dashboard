#pragma once

#include <stdint.h>

#include "wake_coordinator.h"

WakeSignal decodeGpioWakeSignal(uint64_t gpioWakeStatus,
                                uint8_t bootPin,
                                uint8_t userPin,
                                bool bootPressedNow,
                                bool userPressedNow);
