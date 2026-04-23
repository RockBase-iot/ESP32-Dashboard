#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <GxEPD2_3C.h>
#include "drivers/sensor/aht20/Aht20Sensor.h"
#include <esp_sleep.h>

#include "bsp/IBoard.h"
#include "config.h"

// ─── EPD display object ─────────────────────────────────────────────────────
// 4.2" Red/Black/White EPD (GxEPD2_420c_GDEY042Z98), 400×300 px.
// GxEPD2 1.6.x: pins are passed to the driver class, which is then wrapped.
static GxEPD2_420c_GDEY042Z98 _epd_driver(
    /*CS=*/   PIN_EPD_CS,
    /*DC=*/   PIN_EPD_DC,
    /*RST=*/  PIN_EPD_RST,
    /*BUSY=*/ PIN_EPD_BUSY
);
static GxEPD2_3C<GxEPD2_420c_GDEY042Z98,
                 GxEPD2_420c_GDEY042Z98::HEIGHT / 2> _display(_epd_driver);

// ─── EpdDriver adapter ────────────────────────────────────────────────────
class EpdDriver final : public IEpdDriver {
public:
    void init(bool initialPowerOn) override {
        SPI.begin(PIN_EPD_SCK, PIN_EPD_MISO, PIN_EPD_MOSI, PIN_EPD_CS);
        _display.init(115200, initialPowerOn);
    }
    void hibernate() override { _display.hibernate(); }
    void firstPage() override { _display.firstPage(); }
    bool nextPage() override  { return _display.nextPage(); }
};

// ─── Board implementation ─────────────────────────────────────────────────
class Board final : public IBoard {
public:
    void init() override {
        Serial.begin(115200);
        // PIN_EPD_PWR is wired directly to 3.3 V on this board; no switching needed.
    }

    IEpdDriver   &epd()          override { return _epd; }
    Adafruit_GFX &gfx()          override { return _display; }
    uint16_t      dispWidth()  const override { return DISP_WIDTH; }
    uint16_t      dispHeight() const override { return DISP_HEIGHT; }
    uint16_t      colorBlack() const override { return GxEPD_BLACK; }
    uint16_t      colorWhite() const override { return GxEPD_WHITE; }
    uint16_t      colorAccent()    const override { return GxEPD_RED; }
    bool          hasAccentColor() const override { return true; }
    ISensor      *getTempSensor()   override { return &_sensor; }

    uint32_t readBatteryMv() override {
        // 100 kΩ + 100 kΩ voltage divider; 12-bit ADC, 3.3 V reference.
        uint32_t raw = analogRead(PIN_BAT_ADC);
        return static_cast<uint32_t>(raw * 3300UL * 2 / 4095);
    }

    void deepSleep(uint64_t microseconds) override {
        esp_sleep_enable_timer_wakeup(microseconds);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // Boot button (IO0) wakes deep sleep
        esp_deep_sleep_start();
    }

    uint8_t bootButtonPin() const override { return PIN_BOOT_BTN; }
    uint8_t apButtonPin()   const override { return PIN_AP_BTN; }

    const char *boardName() const override { return "NM-Display-420"; }

private:
    EpdDriver    _epd;
    // AHT20: TEMP_CTL (GPIO40) is driven HIGH in Aht20Sensor::begin().
    Aht20Sensor  _sensor{PIN_TEMP_SDA, PIN_TEMP_SCL, PIN_TEMP_CTL};
};

// ─── Singleton accessor ───────────────────────────────────────────────────
IBoard &getBoard() {
    static Board board;
    return board;
}
