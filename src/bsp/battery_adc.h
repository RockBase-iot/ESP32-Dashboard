#pragma once

// ─── Shared battery-ADC sampling contract ───────────────────────────────────
//
// Every board feeds its battery divider into a gated ADC input. The sampling
// sequence is identical everywhere:
//
//   1. Raise the divider-circuit enable pin (the network is disconnected
//      otherwise, so the ADC input would float).
//   2. Wait BATT_ADC_SETTLE_MS for the divider network and its decoupling to
//      settle. 5 ms was empirically too short: the RC formed by the divider
//      and the ADC's sample-and-hold cap had not finished charging, so the
//      reading came out low.
//   3. Attach the pin as an ADC channel, then set its attenuation. This is
//      mandatory, and the order matters — see below.
//   4. Average BATT_ADC_SAMPLES readings.
//   5. Drop the enable pin again so the divider stops drawing current.
//
// Step 3 is available as the single batteryAdcConfigure() helper; the two
// underlying steps are only exposed separately for callers that need them.
//
// Why the ADC must be configured explicitly:
//   A 1S Li-ion through a 1:1 divider presents up to ~2100 mV at the ADC pin.
//   ADC_11db covers that range; lower attenuation settings can saturate. The
//   framework's current default is 11 dB, but this must not depend on a
//   framework default or another component's global ADC configuration.
//
// Use analogReadMilliVolts() rather than deriving millivolts from a raw code.
// It applies the ESP32 ADC calibration data and avoids a systematic error from
// treating the nominal 11 dB full-scale voltage as an exact per-device value.
//
// ── Ordering trap: attach the pin BEFORE setting its attenuation ────────────
// analogSetPinAttenuation(pin, ...) reaches __analogChannelConfig(width, atten,
// pin) with pin != -1, which takes the "reconfigure single channel" branch of
// esp32-hal-adc.c and *requires the pin to already be registered as
// ESP32_BUS_TYPE_ADC_ONESHOT*. On a fresh boot the pin is not registered yet —
// Board::init() leaves it as a plain INPUT — so the call does nothing but print
//
//     [E] __analogChannelConfig(): Pin is not configured as analog channel
//
// The registration happens inside the framework's own analogRead()/
// analogReadMilliVolts() path. So the attenuation has to be set *after* a
// priming read, which is why batteryAdcConfigure() performs an attach step
// before the attenuation step.

#include <Arduino.h>
#include <stdint.h>

// Divider ratio is board-specific (BATT_ADC_DIV in each board's config.h).
// These sampling parameters are common to every board.
static constexpr uint8_t  BATT_ADC_SAMPLES    = 8;    // averaged readings
static constexpr uint8_t  BATT_ADC_SAMPLE_GAP = 1;    // ms between readings
static constexpr uint16_t BATT_ADC_SETTLE_MS  = 20;   // divider settle delay

// Register `pin` as an ADC oneshot channel. A throwaway read is the only
// public way to make the framework attach the pin; its value is discarded.
// Must run before batteryAdcSetAttenuation().
inline void batteryAdcAttachPin(uint8_t pin) {
    (void)analogReadMilliVolts(pin);
}

// Set the attenuation for an already-attached pin. Safe to call repeatedly.
inline void batteryAdcSetAttenuation(uint8_t pin) {
    analogSetPinAttenuation(pin, ADC_11db);
}

// Step 3 in one call: attach first, then set attenuation. Calling these in the
// wrong order is the whole trap described above, so prefer this helper.
// Idempotent; call it on every read so the setting survives any other code that
// reconfigures the ADC.
inline void batteryAdcConfigure(uint8_t pin) {
    batteryAdcAttachPin(pin);
    batteryAdcSetAttenuation(pin);
}

// Average calibrated ADC-pin millivolt readings on an already-enabled,
// already-settled input. Kept separate from the enable/settle sequence so each
// board can keep its own pin-driving style.
inline uint32_t batteryAdcAverageMilliVolts(uint8_t pin) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < BATT_ADC_SAMPLES; ++i) {
        sum += analogReadMilliVolts(pin);
        delay(BATT_ADC_SAMPLE_GAP);
    }
    return sum / BATT_ADC_SAMPLES;
}

// Convert calibrated ADC-pin millivolts to battery-terminal millivolts.
inline uint32_t batteryAdcToBatteryMv(uint32_t adcMilliVolts, uint8_t dividerRatio) {
    return adcMilliVolts * dividerRatio;
}
