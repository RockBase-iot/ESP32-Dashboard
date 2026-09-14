#pragma once

#include <stdint.h>

// NM-EPD-420-4C (GDEY0420F51 / HX8717) board contract.
static constexpr uint16_t DISP_WIDTH = 400;
static constexpr uint16_t DISP_HEIGHT = 300;
static constexpr uint8_t EPD_COLOR_COUNT = 4;
static constexpr uint32_t EPD_FRAME_BYTES =
    static_cast<uint32_t>(DISP_WIDTH) * DISP_HEIGHT / 4U;

static constexpr uint8_t PIN_EPD_SCK = 2;
static constexpr uint8_t PIN_EPD_MOSI = 1;
static constexpr int8_t PIN_EPD_MISO = -1;
static constexpr uint8_t PIN_EPD_CS = 46;
static constexpr uint8_t PIN_EPD_DC = 4;
static constexpr uint8_t PIN_EPD_RST = 5;
static constexpr uint8_t PIN_EPD_BUSY = 6;

static constexpr uint8_t PIN_BOOT_BTN = 0;
static constexpr uint8_t PIN_USER_BTN = 45;

static constexpr uint8_t PIN_BATT_ADC = 3;
static constexpr uint8_t PIN_ADC_EN = 43;
static constexpr uint8_t BATT_ADC_DIV = 2;

static constexpr uint8_t PIN_TEMP_SDA = 39;
static constexpr uint8_t PIN_TEMP_SCL = 38;
static constexpr uint8_t PIN_TEMP_CTL = 40;
static constexpr uint8_t PIN_PA_CTRL = 41;
static constexpr uint8_t PIN_CODEC_EN = 44;
static constexpr uint8_t PIN_LORA_EN = 47;

static constexpr uint8_t PIN_LORA_NSS = 8;
static constexpr uint8_t PIN_LORA_SCK = 9;
static constexpr uint8_t PIN_LORA_MOSI = 10;
static constexpr uint8_t PIN_LORA_MISO = 11;
static constexpr uint8_t PIN_LORA_RST = 12;
static constexpr uint8_t PIN_LORA_BUSY = 13;
static constexpr uint8_t PIN_LORA_DIO1 = 14;

static constexpr uint8_t PIN_I2S_SCLK = 15;
static constexpr uint8_t PIN_I2S_ASDOUT = 16;
static constexpr uint8_t PIN_I2S_LRCK = 17;
static constexpr uint8_t PIN_I2S_DSIN = 18;
static constexpr uint8_t PIN_I2S_MCLK = 21;
static constexpr uint8_t PIN_TF_CS = 7;

