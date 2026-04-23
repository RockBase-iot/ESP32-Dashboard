#pragma once

#include <Adafruit_GFX.h>
#include <stdint.h>

#include "drivers/sensor/ISensor.h"

// ─── EPD driver interface ───────────────────────────────────────────────────
class IEpdDriver {
public:
    virtual ~IEpdDriver() = default;

    // Initialize the display. Call once before any drawing.
    virtual void init(bool initialPowerOn = true) = 0;

    // Put display into deep sleep mode (call before MCU deep sleep).
    virtual void hibernate() = 0;

    // Begin a page-based rendering pass (call before the draw loop).
    virtual void firstPage() = 0;

    // Advance to next page; returns false when all pages are done.
    virtual bool nextPage() = 0;
};

// ─── Board interface ────────────────────────────────────────────────────────
class IBoard {
public:
    virtual ~IBoard() = default;

    // Board-level initialization (serial port, power rails, peripherals).
    // Must be called once at the very start of run(), before any other method.
    virtual void init() = 0;

    // EPD driver proxy (init, hibernate, page-based rendering).
    virtual IEpdDriver &epd() = 0;

    // Adafruit_GFX drawing context (GxEPD2 inherits from it).
    virtual Adafruit_GFX &gfx() = 0;

    // Display resolution.
    virtual uint16_t dispWidth()  const = 0;
    virtual uint16_t dispHeight() const = 0;

    // Color palette.
    virtual uint16_t colorBlack()  const = 0;
    virtual uint16_t colorWhite()  const = 0;

    // Accent color (red/yellow on 3-color panels; black on BW panels).
    virtual uint16_t colorAccent()    const = 0;
    virtual bool     hasAccentColor() const = 0;

    // Optional temperature/humidity sensor; returns nullptr if not present.
    virtual ISensor *getTempSensor() = 0;

    // Battery voltage in millivolts (0 if not measurable).
    virtual uint32_t readBatteryMv() = 0;

    // Enter deep sleep for the given duration.
    virtual void deepSleep(uint64_t microseconds) = 0;

    // GPIO pin numbers for physical buttons (external pull-up, pressed = LOW).
    // Returns 0xFF if this board does not have the button.
    virtual uint8_t bootButtonPin() const = 0;  // IO0 / BOOT key (RTC GPIO, can wake deep sleep)
    virtual uint8_t apButtonPin()   const = 0;  // User-defined AP config key

    // Short human-readable board identifier, e.g. "FireBeetle2-ESP32E".
    virtual const char *boardName() const = 0;
};

// Each BSP's Board.cpp implements and returns the singleton board instance.
IBoard &getBoard();
