#pragma once

#include <stdint.h>

// ─── FireBeetle ESP32 (micro-USB variant) — board config ──────────────────

// Display resolution
static constexpr uint16_t DISP_WIDTH  = 800;
static constexpr uint16_t DISP_HEIGHT = 480;

// ─── Pinout ───────────────────────────────────────────────────────────────

// ADC pin used to measure battery voltage.
// Unlike the NM boards this input is not gated by an enable pin: the FireBeetle
// divider is permanently connected, so there is nothing to power up or down.
static constexpr uint8_t PIN_BAT_ADC  = A0;
static constexpr uint8_t BATT_ADC_DIV = 2;   // Divider ratio: ADC_mV × BATT_ADC_DIV = battery mV

// SPI pins for E-Paper Driver Board.
static constexpr uint8_t PIN_EPD_BUSY =  5;
static constexpr uint8_t PIN_EPD_CS   = 13;
static constexpr uint8_t PIN_EPD_RST  = 21;
static constexpr uint8_t PIN_EPD_DC   = 22;
static constexpr uint8_t PIN_EPD_SCK  = 18;
static constexpr uint8_t PIN_EPD_MISO = 19; // Not used (display is write-only)
static constexpr uint8_t PIN_EPD_MOSI = 23;
static constexpr uint8_t PIN_EPD_PWR  = 26; // Irrelevant if wired to 3.3 V

// I2C pins for BME280 / BME680.
static constexpr uint8_t PIN_BME_SDA  = 17;
static constexpr uint8_t PIN_BME_SCL  = 16;
static constexpr uint8_t PIN_BME_PWR  =  4; // Irrelevant if wired to 3.3 V
