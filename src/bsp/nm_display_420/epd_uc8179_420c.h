// ─── UC8179-based 4.2" 3-color EPD driver ──────────────────────────────────
// Adapted from GxEPD2_750c_GDEY075Z08 (UC8179, 3C, 800x480) for 400×300.
// This driver is used when the nm-display-420 board carries a UC8179-driven
// panel instead of the default SSD1683 (GDEY042Z98).
//
// Protocol: UC8179 command set with 3-color buffer mapping:
//   0x10 = previous(b/w) buffer,  0x13 = current(red/color) buffer
//   Full refresh only (no fast partial update).

#pragma once

#include <GxEPD2_EPD.h>

class GxEPD2_420c_NM_UC8179 : public GxEPD2_EPD
{
public:
    static constexpr uint16_t WIDTH  = 400;
    static constexpr uint16_t WIDTH_VISIBLE = WIDTH;
    static constexpr uint16_t HEIGHT = 300;
    static constexpr GxEPD2::Panel panel = GxEPD2::GDEY042Z98; // same panel spec
    static constexpr bool hasColor              = true;
    static constexpr bool hasPartialUpdate       = false;
    static constexpr bool hasFastPartialUpdate   = false;
    static constexpr bool useFastFullUpdate      = false;
    static constexpr uint16_t power_on_time      = 150;   // ms
    static constexpr uint16_t power_off_time     = 50;    // ms
    static constexpr uint16_t full_refresh_time  = 18000; // ms (4.2" ~18s)
    static constexpr uint16_t partial_refresh_time = 18000;

    GxEPD2_420c_NM_UC8179(int16_t cs, int16_t dc, int16_t rst, int16_t busy);

    // ── GxEPD2_EPD virtual overrides ──────────────────────────────────────
    void clearScreen(uint8_t value = 0xFF);
    void clearScreen(uint8_t black_value, uint8_t color_value);
    void writeScreenBuffer(uint8_t value = 0xFF);
    void writeScreenBuffer(uint8_t black_value, uint8_t color_value);

    void writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h,
                    bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part,
                        int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h,
                        bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImage(const uint8_t* black, const uint8_t* color,
                    int16_t x, int16_t y, int16_t w, int16_t h,
                    bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t* black, const uint8_t* color,
                        int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h,
                        bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeNative(const uint8_t* data1, const uint8_t* data2,
                     int16_t x, int16_t y, int16_t w, int16_t h,
                     bool invert = false, bool mirror_y = false, bool pgm = false);

    void drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h,
                   bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part,
                       int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h,
                       bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImage(const uint8_t* black, const uint8_t* color,
                   int16_t x, int16_t y, int16_t w, int16_t h,
                   bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t* black, const uint8_t* color,
                       int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h,
                       bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawNative(const uint8_t* data1, const uint8_t* data2,
                    int16_t x, int16_t y, int16_t w, int16_t h,
                    bool invert = false, bool mirror_y = false, bool pgm = false);

    void refresh(bool partial_update_mode = false);
    void refresh(int16_t x, int16_t y, int16_t w, int16_t h);
    void powerOff();
    void hibernate();

private:
    void _writeScreenBuffer(uint8_t command, uint8_t value);
    void _writeImage(uint8_t command, const uint8_t bitmap[],
                     int16_t x, int16_t y, int16_t w, int16_t h,
                     bool invert, bool mirror_y, bool pgm);
    void _writeImagePart(uint8_t command, const uint8_t bitmap[],
                         int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                         int16_t x, int16_t y, int16_t w, int16_t h,
                         bool invert, bool mirror_y, bool pgm);
    void _setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _PowerOn();
    void _PowerOff();
    void _InitDisplay();
    void _Update_Full();
};
