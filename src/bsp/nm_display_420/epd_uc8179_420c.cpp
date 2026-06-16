// ─── UC8179-based 4.2" 3-color EPD driver ──────────────────────────────────
// Adapted from GxEPD2_750c_GDEY075Z08 for 400×300.

#include "epd_uc8179_420c.h"

// ─── Constructor ──────────────────────────────────────────────────────────
GxEPD2_420c_NM_UC8179::GxEPD2_420c_NM_UC8179(int16_t cs, int16_t dc, int16_t rst, int16_t busy)
    : GxEPD2_EPD(cs, dc, rst, busy, LOW, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
}

// ─── Screen buffer helpers ────────────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::clearScreen(uint8_t value)
{
    clearScreen(value, 0xFF);
}

void GxEPD2_420c_NM_UC8179::clearScreen(uint8_t black_value, uint8_t color_value)
{
    writeScreenBuffer(black_value, color_value);
    refresh(false);
}

void GxEPD2_420c_NM_UC8179::writeScreenBuffer(uint8_t value)
{
    writeScreenBuffer(value, 0xFF);
}

void GxEPD2_420c_NM_UC8179::writeScreenBuffer(uint8_t black_value, uint8_t color_value)
{
    if (!_init_display_done) _InitDisplay();
    // UC8179 3C: 0x10 = B/W previous buffer, 0x13 = RED current buffer
    _writeScreenBuffer(0x10, black_value);
    _writeScreenBuffer(0x13, color_value);
    _initial_write = false;
}

void GxEPD2_420c_NM_UC8179::_writeScreenBuffer(uint8_t command, uint8_t value)
{
    _setPartialRamArea(0, 0, WIDTH, HEIGHT);
    _writeCommand(command);
    _startTransfer();
    for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
        _transfer(value);
    _endTransfer();
}

// ─── Image writing ────────────────────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::writeImage(const uint8_t bitmap[], int16_t x, int16_t y,
                                        int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
    _writeScreenBuffer(0x13, 0xFF); // clear red plane
    _writeImage(0x10, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_420c_NM_UC8179::_writeImage(uint8_t command, const uint8_t bitmap[],
                                         int16_t x, int16_t y, int16_t w, int16_t h,
                                         bool invert, bool mirror_y, bool pgm)
{
    if (_initial_write) writeScreenBuffer();
    delay(1);
    uint16_t wb = (w + 7) / 8;
    x -= x % 8;
    w = wb * 8;
    int16_t x1 = x < 0 ? 0 : x;
    int16_t y1 = y < 0 ? 0 : y;
    int16_t w1 = x + w < int16_t(WIDTH)  ? w : int16_t(WIDTH)  - x;
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y;
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx; h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    if (!_init_display_done) _InitDisplay();
    _writeCommand(0x91); // partial in
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(command);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++) {
        for (int16_t j = 0; j < w1 / 8; j++) {
            uint8_t data = 0xFF;
            uint16_t idx = mirror_y
                ? j + dx / 8 + uint16_t((h - 1 - (i + dy))) * wb
                : j + dx / 8 + uint16_t(i + dy) * wb;
            if (pgm) {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = pgm_read_byte(&bitmap[idx]);
#else
                data = bitmap[idx];
#endif
            } else {
                data = bitmap[idx];
            }
            if (invert) data = ~data;
            _transfer(data);
        }
    }
    _endTransfer();
    _writeCommand(0x92); // partial out
    delay(1);
}

void GxEPD2_420c_NM_UC8179::writeImagePart(const uint8_t bitmap[],
        int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    _writeImagePart(0x10, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_420c_NM_UC8179::_writeImagePart(uint8_t command, const uint8_t bitmap[],
        int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    if (_initial_write) writeScreenBuffer();
    delay(1);
    if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
    if ((x_part < 0) || (x_part >= w_bitmap)) return;
    if ((y_part < 0) || (y_part >= h_bitmap)) return;
    uint16_t wb_bitmap = (w_bitmap + 7) / 8;
    x_part -= x_part % 8;
    w = w_bitmap - x_part < w ? w_bitmap - x_part : w;
    h = h_bitmap - y_part < h ? h_bitmap - y_part : h;
    x -= x % 8;
    w = 8 * ((w + 7) / 8);
    int16_t x1 = x < 0 ? 0 : x;
    int16_t y1 = y < 0 ? 0 : y;
    int16_t w1 = x + w < int16_t(WIDTH)  ? w : int16_t(WIDTH)  - x;
    int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y;
    int16_t dx = x1 - x;
    int16_t dy = y1 - y;
    w1 -= dx; h1 -= dy;
    if ((w1 <= 0) || (h1 <= 0)) return;
    if (!_init_display_done) _InitDisplay();
    _writeCommand(0x91);
    _setPartialRamArea(x1, y1, w1, h1);
    _writeCommand(command);
    _startTransfer();
    for (int16_t i = 0; i < h1; i++) {
        for (int16_t j = 0; j < w1 / 8; j++) {
            uint8_t data;
            uint32_t idx = mirror_y
                ? x_part / 8 + j + dx / 8 + uint32_t((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap
                : x_part / 8 + j + dx / 8 + uint32_t(y_part + i + dy) * wb_bitmap;
            if (pgm) {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = pgm_read_byte(&bitmap[idx]);
#else
                data = bitmap[idx];
#endif
            } else {
                data = bitmap[idx];
            }
            if (invert) data = ~data;
            _transfer(data);
        }
    }
    _endTransfer();
    _writeCommand(0x92);
    delay(1);
}

// ─── Dual-plane write ─────────────────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::writeImage(const uint8_t* black, const uint8_t* color,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    if (black) _writeImage(0x10, black, x, y, w, h, invert, mirror_y, pgm);
    if (color) _writeImage(0x13, color, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_420c_NM_UC8179::writeImagePart(const uint8_t* black, const uint8_t* color,
        int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    if (black) _writeImagePart(0x10, black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
    if (color) _writeImagePart(0x13, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_420c_NM_UC8179::writeNative(const uint8_t* data1, const uint8_t* data2,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    if (data1) writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
}

// ─── Draw (write + refresh) ───────────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::drawImage(const uint8_t bitmap[], int16_t x, int16_t y,
                                       int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
    writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
    refresh(x, y, w, h);
}

void GxEPD2_420c_NM_UC8179::drawImagePart(const uint8_t bitmap[],
        int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
    refresh(x, y, w, h);
}

void GxEPD2_420c_NM_UC8179::drawImage(const uint8_t* black, const uint8_t* color,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    writeImage(black, color, x, y, w, h, invert, mirror_y, pgm);
    refresh(x, y, w, h);
}

void GxEPD2_420c_NM_UC8179::drawImagePart(const uint8_t* black, const uint8_t* color,
        int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    writeImagePart(black, color, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
    refresh(x, y, w, h);
}

void GxEPD2_420c_NM_UC8179::drawNative(const uint8_t* data1, const uint8_t* data2,
        int16_t x, int16_t y, int16_t w, int16_t h,
        bool invert, bool mirror_y, bool pgm)
{
    writeNative(data1, data2, x, y, w, h, invert, mirror_y, pgm);
    refresh(x, y, w, h);
}

// ─── Refresh / Power / Hibernate ──────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::refresh(bool partial_update_mode)
{
    if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
    else _Update_Full();
}

void GxEPD2_420c_NM_UC8179::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
    // Simplify: always full refresh; partial refresh needs tuned LUTs.
    _Update_Full();
    (void)x; (void)y; (void)w; (void)h;
}

void GxEPD2_420c_NM_UC8179::powerOff()
{
    _PowerOff();
}

void GxEPD2_420c_NM_UC8179::hibernate()
{
    _PowerOff();
    if (_rst >= 0) {
        _writeCommand(0x07); // deep sleep
        _writeData(0xA5);    // check code
        _hibernating = true;
        _init_display_done = false;
    }
}

// ─── Hardware helpers ─────────────────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive
    uint16_t ye = y + h - 1;
    x &= 0xFFF8;
    _writeCommand(0x90); // partial window
    _writeData(x  / 256);
    _writeData(x  % 256);
    _writeData(xe / 256);
    _writeData(xe % 256);
    _writeData(y  / 256);
    _writeData(y  % 256);
    _writeData(ye / 256);
    _writeData(ye % 256);
    _writeData(0x00);
}

void GxEPD2_420c_NM_UC8179::_PowerOn()
{
    if (!_power_is_on) {
        _writeCommand(0x04);
        _waitWhileBusy("_PowerOn", power_on_time);
    }
    _power_is_on = true;
}

void GxEPD2_420c_NM_UC8179::_PowerOff()
{
    if (_power_is_on) {
        _writeCommand(0x02);
        _waitWhileBusy("_PowerOff", power_off_time);
        _power_is_on = false;
    }
}

// ─── Init ─────────────────────────────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::_InitDisplay()
{
    if (_hibernating) _reset();
    // UC8179 initialisation sequence for 4.2" 400×300 3-color panel
    _writeCommand(0x01); // POWER SETTING
    _writeData(0x07);
    _writeData(0x07);   // VGH=20V, VGL=-20V
    _writeData(0x3f);   // VDH=15V
    _writeData(0x3f);   // VDL=-15V
    _writeCommand(0x06); // Booster Soft Start
    _writeData(0x17);
    _writeData(0x17);
    _writeData(0x28);
    _writeData(0x17);
    _writeCommand(0x00); // PANEL SETTING
    _writeData(0x0f);    // LUT from OTP
    _writeCommand(0x61); // TRES (resolution)
    _writeData(WIDTH  / 256);  // 400 → 0x01
    _writeData(WIDTH  % 256);  // 400 → 0x90
    _writeData(HEIGHT / 256);  // 300 → 0x01
    _writeData(HEIGHT % 256);  // 300 → 0x2C
    _writeCommand(0x15); // DUSPI
    _writeData(0x00);    // disabled
    _writeCommand(0x50); // VCOM AND DATA INTERVAL SETTING
    _writeData(0x13);    // border floating
    _writeData(0x07);    // CDI
    _writeCommand(0x60); // TCON SETTING
    _writeData(0x22);
    _init_display_done = true;
}

// ─── Update ───────────────────────────────────────────────────────────────
void GxEPD2_420c_NM_UC8179::_Update_Full()
{
    _writeCommand(0x00); // PANEL SETTING
    _writeData(0x0f);    // LUT from OTP
    _writeCommand(0x50); // VCOM AND DATA INTERVAL SETTING
    _writeData(0x13);
    _writeData(0x07);
    // Use internal temperature sensor (no forced temperature)
    _writeCommand(0xE0); // Cascade Setting
    _writeData(0x00);    // no TSFIX
    _writeCommand(0x41); // TSE
    _writeData(0x00);
    _PowerOn();
    _writeCommand(0x12); // display refresh
    _waitWhileBusy("_Update_Full", full_refresh_time);
    _PowerOff();
}
