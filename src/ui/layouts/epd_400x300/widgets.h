#pragma once

#include <stdint.h>

// ─── Layout constants for 400×300 EPD (4.2" 3-color) ─────────────────────
// Coordinates derived from the reference project's DISP_3C_E420 sections
// in renderer.cpp.  Origin top-left (0,0).

// ── Display dimensions ──
static constexpr int16_t DISP_W = 400;
static constexpr int16_t DISP_H = 300;

// ── Left panel width (data rows) / right panel start ──
static constexpr int16_t LEFT_PANEL_W  = 175; // data rows live in x: 0..174
static constexpr int16_t RIGHT_PANEL_X = 178; // forecast / graph start

// ── Current conditions (top-left 96×96 icon + temp) ──
static constexpr int16_t CURRENT_ICON_X    =   0;
static constexpr int16_t CURRENT_ICON_Y    =   0;
static constexpr int16_t CURRENT_ICON_SIZE =  96; // pixels (96×96 bitmap)

// Temperature text (CENTER-aligned around x=116, baseline y=60)
// Reference: drawString(96+30-10, 96/2+25/2, …, CENTER) → (116, 60)
static constexpr int16_t CURRENT_TEMP_X    = 116;
static constexpr int16_t CURRENT_TEMP_Y    =  60;

// Degree unit – drawn at cursor_x after temp, y = 96/2-25/2+10 = 46
static constexpr int16_t CURRENT_UNIT_Y    =  46;

// Feels-like – CENTER around x=126, baseline y=74
// Reference: drawString(96+60/2, 96/2+25/2+12/2+17/2, …, CENTER) → (126, 74)
static constexpr int16_t CURRENT_FEELS_X   = 126;
static constexpr int16_t CURRENT_FEELS_Y   =  74;

// ── Location / date (top-right, right-aligned from x=DISP_W-2) ──
static constexpr int16_t LOC_X        = DISP_W - 2;  // 398
static constexpr int16_t LOC_CITY_Y   =  12;          // baseline
static constexpr int16_t LOC_DATE_Y   =  25;          // baseline (15+2+8)

// ── Data rows (left panel, y starting at 104, row height = 24+6 = 30) ──
// Left column: icon x=0, label x=24, value x=24 + half row
// Right column: icon x=85 (=170/2), label x=105 (=170/2+20), value x=109 (=170/2+24)
static constexpr int16_t DATA_ROW_BASE   = 104;
static constexpr int16_t DATA_ROW_H      =  30; // 24 icon + 6 gap
static constexpr int16_t DATA_ICON_L_X   =   0;
static constexpr int16_t DATA_LABEL_L_X  =  24;
static constexpr int16_t DATA_VALUE_L_X  =  24;
static constexpr int16_t DATA_ICON_R_X   =  85; // 170/2
static constexpr int16_t DATA_LABEL_R_X  = 105; // 170/2+20
static constexpr int16_t DATA_VALUE_R_X  = 109; // 170/2+24

// Inline helpers for row y positions (icon top, label, value baseline)
// icon_y(i)  = DATA_ROW_BASE + DATA_ROW_H * i
// label_y(i) = DATA_ROW_BASE + 5 + DATA_ROW_H * i        (small label above value)
// value_y(i) = DATA_ROW_BASE + 17 + DATA_ROW_H * i       (= +5+24/2)

// ── Forecast columns (right panel, y=47 icon, y=42 day, y=88 hi/lo) ──
// Reference: x = 178 + i*44, 5 columns; we use 4 days (today+3)
static constexpr int16_t FC_COL_W         =  44; // column stride
static constexpr int16_t FC_ICON_Y        =  47; // 49+69/4-32/2-6/2
static constexpr int16_t FC_ICON_SIZE     =  32;
static constexpr int16_t FC_DAY_Y         =  42; // label above icon
static constexpr int16_t FC_HILO_Y        =  88; // hi | lo row
static constexpr int16_t FC_PRECIP_Y      = 100; // precipitation below hi/lo
// center offset within 44-px column: x + 15 → center of text/icon
static constexpr int16_t FC_COL_CX        =  15;

// ── Hourly graph (right panel lower area) ──
static constexpr int16_t GRAPH_X0         = LEFT_PANEL_W; // 175
static constexpr int16_t GRAPH_X1         = DISP_W;       // 400 (adjusted for labels)
static constexpr int16_t GRAPH_Y0         = 108;
static constexpr int16_t GRAPH_Y1         = DISP_H - 46;  // 254

// ── Status bar (bottom strip) ──
// Text baseline at y = DISP_H-1-2 = 297
// 24×24 icons top at y = DISP_H-1-17 = 282
// 16×16 icons top at y = DISP_H-1-13 = 286
static constexpr int16_t STATUS_TEXT_Y    = DISP_H - 3;   // 297
static constexpr int16_t STATUS_ICON24_Y  = DISP_H - 18;  // 282
static constexpr int16_t STATUS_ICON16_Y  = DISP_H - 14;  // 286
