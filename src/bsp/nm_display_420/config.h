#pragma once

#include <stdint.h>

// ─── NM Display 420 — board config ────────────────────────────────────────

// Display resolution
static constexpr uint16_t DISP_WIDTH  = 400;
static constexpr uint16_t DISP_HEIGHT = 300;

// ─── Pinout ───────────────────────────────────────────────────────────────

// ADC pin used to measure battery voltage.
static constexpr uint8_t PIN_BAT_ADC  = A0;

// SPI pins for E-Paper Driver Board.
static constexpr uint8_t PIN_EPD_BUSY = 6;
static constexpr uint8_t PIN_EPD_CS   = 3;
static constexpr uint8_t PIN_EPD_RST  = 5;
static constexpr uint8_t PIN_EPD_DC   = 4;
static constexpr uint8_t PIN_EPD_SCK  = 2;
static constexpr uint8_t PIN_EPD_MISO = 10; // Not used (display is write-only)
static constexpr uint8_t PIN_EPD_MOSI = 1;
static constexpr uint8_t PIN_EPD_PWR  = 21; // Irrelevant if wired to 3.3 V

// I2C pins for on-board temperature/humidity sensor (AHT20).
// TEMP_CTL (GPIO40) is a power-enable pin: drive HIGH before accessing the sensor.
static constexpr int PIN_TEMP_SDA  = 39;
static constexpr int PIN_TEMP_SCL  = 38;
static constexpr int PIN_TEMP_CTL  = 40;

// Physical buttons (external pull-up to 3.3 V; pressed = LOW).
static constexpr uint8_t PIN_BOOT_BTN = 0;   // IO0  — Boot/BOOT key (RTC GPIO, wakes deep sleep)
static constexpr uint8_t PIN_AP_BTN   = 45;  // IO45 — User AP config key
