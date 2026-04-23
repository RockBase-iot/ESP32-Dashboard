#pragma once

#include <stdint.h>

// ─── Layout constants for 800×480 EPD (7.5" BW/3C/7C) ────────────────────
// All values in pixels. Origin is top-left (0, 0).
// TODO: Adjust all constants once actual artwork / mockup is available.

// ── Display dimensions ──
static constexpr uint16_t DISP_W = 800;
static constexpr uint16_t DISP_H = 480;

// ── Status bar ──
static constexpr int16_t STATUS_BAR_Y      =   0;
static constexpr int16_t STATUS_BAR_H      =  28;

// ── Current conditions area ──
static constexpr int16_t CURRENT_ICON_X    =  16;
static constexpr int16_t CURRENT_ICON_Y    =  40;
static constexpr int16_t CURRENT_ICON_SIZE = 196;

static constexpr int16_t CURRENT_TEMP_X    = 228;
static constexpr int16_t CURRENT_TEMP_Y    =  44;

static constexpr int16_t CURRENT_DESC_X    = 228;
static constexpr int16_t CURRENT_DESC_Y    = 140;

static constexpr int16_t CURRENT_FEELS_X   = 228;
static constexpr int16_t CURRENT_FEELS_Y   = 168;

static constexpr int16_t CURRENT_HUMIDITY_X = 228;
static constexpr int16_t CURRENT_HUMIDITY_Y = 196;

static constexpr int16_t CURRENT_WIND_X    = 228;
static constexpr int16_t CURRENT_WIND_Y    = 224;

static constexpr int16_t CURRENT_PRES_X    = 228;
static constexpr int16_t CURRENT_PRES_Y    = 252;

// ── Sunrise / Sunset ──
static constexpr int16_t SUN_X             = 560;
static constexpr int16_t SUNRISE_Y         =  44;
static constexpr int16_t SUNSET_Y          =  80;

// ── AQI strip ──
static constexpr int16_t AQI_X             = 560;
static constexpr int16_t AQI_Y             = 120;

// ── Forecast row (4 daily columns) ──
static constexpr int16_t FORECAST_Y        = 300;
static constexpr int16_t FORECAST_H        = 180;
static constexpr int16_t FORECAST_COL_W    = 160;
static constexpr int16_t FORECAST_COL0_X   =  40;
static constexpr int16_t FORECAST_COL1_X   = 240;
static constexpr int16_t FORECAST_COL2_X   = 440;
static constexpr int16_t FORECAST_COL3_X   = 640;
static constexpr int16_t FORECAST_ICON_SIZE =  96;
static constexpr int16_t FORECAST_DIVIDER_Y = 296;

// ── Padding / margins ──
static constexpr int16_t MARGIN            =   8;
