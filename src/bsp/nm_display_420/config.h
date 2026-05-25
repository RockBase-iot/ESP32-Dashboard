#pragma once

#include <stdint.h>

// ─── NM Display 420 — board config ────────────────────────────────────────

// Display resolution
static constexpr uint16_t DISP_WIDTH  = 400;
static constexpr uint16_t DISP_HEIGHT = 300;

// ─── Pinout ───────────────────────────────────────────────────────────────

// Battery voltage ADC — rev2: IO3 is the ADC input, gated by PIN_ADC_EN.
static constexpr uint8_t PIN_BATT_ADC  = 3;   // IO3  — Battery voltage sense (ADC1_CH2)
static constexpr uint8_t PIN_ADC_EN    = 43;  // IO43 — Battery ADC circuit enable (HIGH = on)
static constexpr uint8_t BATT_ADC_DIV  = 2;   // Divider ratio: ADC_mV × BATT_ADC_DIV = battery mV

// SPI pins for E-Paper Driver Board.
static constexpr uint8_t PIN_EPD_BUSY = 6;
static constexpr uint8_t PIN_EPD_CS   = 46; // IO46 — rev2: IO3 freed for battery ADC
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

// External peripheral power-control pins.
// Deep-sleep latch targets: PA_CTRL→LOW, LORA_EN→LOW, CODEC_EN→LOW, ADC_EN→LOW,
//   TEMP_CTL→LOW, LORA_RST→LOW, LORA_NSS→HIGH.
static constexpr uint8_t PIN_PA_CTRL  = 41;  // IO41 — Power Amplifier enable (HIGH = on)
static constexpr uint8_t PIN_LORA_EN  = 47;  // IO47 — LoRa module power enable (HIGH = on) [rev2]
static constexpr uint8_t PIN_CODEC_EN = 44;  // IO44 — ES8311 codec power enable (HIGH = on) [rev2]
static constexpr uint8_t PIN_LORA_RST = 12;  // IO12 — LoRa module reset   (LOW  = reset)
static constexpr uint8_t PIN_LORA_NSS = 8;   // IO8  — LoRa SPI chip-select (HIGH = deselected)
