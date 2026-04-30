#pragma once
// ES8311 Codec — suspend / power-down helper
//
// The NM-Display-420 board carries an ES8311 audio codec on the shared I2C
// bus (SDA=GPIO39, SCL=GPIO38, addr=0x18).  When audio is not used the chip
// should be put into its lowest-power suspend state immediately after power-on
// to avoid unnecessary current draw (~3 mA idle → <10 µA suspended).
//
// Register sequence mirrors the Espressif ES8311 driver suspend flow used in
// the companion factory-test firmware (NM-Display-420/src/tests/test_t4_codec.h).

#include <Wire.h>

static constexpr uint8_t ES8311_I2C_ADDR = 0x18;

// Send ES8311 into suspend / power-down state.
//
// @param wire  TwoWire bus that has already been initialised (Wire.begin called).
// @param addr  I2C address (default 0x18).
inline void es8311_enter_powerdown(TwoWire &wire, uint8_t addr = ES8311_I2C_ADDR) {
    auto wr = [&](uint8_t reg, uint8_t val) {
        wire.beginTransmission(addr);
        wire.write(reg);
        wire.write(val);
        wire.endTransmission();
    };

    wr(0x32, 0x00);  // DAC volume → 0 before powering off
    wr(0x17, 0x00);  // ADC digital volume → 0
    wr(0x0E, 0xFF);  // Power down all system blocks
    wr(0x12, 0x02);  // Clock / signal-path gate
    wr(0x14, 0x00);  // Disable ADC / DMIC path
    wr(0x0D, 0xFA);  // System power-down control
    wr(0x15, 0x00);  // Disable DAC output path
    wr(0x45, 0x01);  // Enter low-power state
}
