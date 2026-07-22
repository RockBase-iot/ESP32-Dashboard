#pragma once

#include <stdint.h>

// Result of one light-sleep episode started by IBoard::lightSleepMs().
//
// Light sleep retains RAM and peripherals, so execution simply continues on
// the next line; this enum tells the caller WHY it woke.
enum class LightWake : uint8_t {
    Timeout = 0,  // maxMs elapsed with no button activity
    BootButton,   // BOOT key caused (or is causing) the wake
    UserButton,   // USER key caused (or is causing) the wake
    Other,        // spurious / unattributable wake, or wake-setup failure
};

// Pure wake classification, separated from hardware so it is unit-testable.
//
// gpioWakeStatus is a merged 64-bit map (GPIO.status | GPIO.status1 << 32,
// optionally OR-ed with ISR-captured bits); bit N corresponds to GPIO N.
// A pin value of 0xFF marks an absent button and never matches.
//
// Buttons win over the timeout: a press landing exactly at the deadline is
// still user intent, and the caller resets its inactivity budget regardless.
LightWake classifyLightWake(bool timerFired, bool gpioFired,
                            uint64_t gpioWakeStatus,
                            uint8_t bootPin, uint8_t userPin,
                            bool bootPressedNow, bool userPressedNow);

// Interactive-window exit policy for the light-sleep loop.
// The window ends only when the inactivity budget is consumed — either
// signalled by a Timeout wake or already overrun when the wake arrives (a
// long render can push elapsed past budget). Button and spurious wakes inside
// the budget simply re-arm the sleep.
bool interactiveWindowShouldExit(LightWake wake, uint32_t elapsedMs, uint32_t budgetMs);
