#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <GxEPD2_BW.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>

#include "bsp/IBoard.h"
#include "config.h"

// ─── EPD display object ────────────────────────────────────────────────────
// 7.5" Black/White EPD (GxEPD2_750_GDEY075T7), 800×480 px.
static GxEPD2_750_GDEY075T7 _epd_driver(
    /*CS=*/   PIN_EPD_CS,
    /*DC=*/   PIN_EPD_DC,
    /*RST=*/  PIN_EPD_RST,
    /*BUSY=*/ PIN_EPD_BUSY
);
static GxEPD2_BW<GxEPD2_750_GDEY075T7,
                 GxEPD2_750_GDEY075T7::HEIGHT> _display(_epd_driver);

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

// ─── BME280 sensor adapter ────────────────────────────────────────────────
class Bme280Sensor final : public ISensor {
public:
    bool begin() override {
        Wire.begin(PIN_BME_SDA, PIN_BME_SCL);
        return _bme.begin();
    }
    bool read(float &temp_c, float &humidity, float &pressure_hpa) override {
        temp_c       = _bme.readTemperature();
        humidity     = _bme.readHumidity();
        pressure_hpa = _bme.readPressure() / 100.0f;
        return true;
    }
    const char *typeName() const override { return "BME280"; }
private:
    Adafruit_BME280 _bme;
};

// ─── Board implementation ─────────────────────────────────────────────────
class Board final : public IBoard {
public:
    void init() override {
        Serial.begin(115200);
        pinMode(PIN_EPD_PWR, OUTPUT);
        digitalWrite(PIN_EPD_PWR, HIGH);
    }

    IEpdDriver   &epd()          override { return _epd; }
    Adafruit_GFX &gfx()          override { return _display; }
    uint16_t      dispWidth()  const override { return DISP_WIDTH; }
    uint16_t      dispHeight() const override { return DISP_HEIGHT; }
    uint16_t      colorBlack() const override { return GxEPD_BLACK; }
    uint16_t      colorWhite() const override { return GxEPD_WHITE; }
    uint16_t      colorAccent()    const override { return GxEPD_BLACK; }
    bool          hasAccentColor() const override { return false; }
    ISensor      *getTempSensor()   override { return &_sensor; }

    uint32_t readBatteryMv() override {
        // 100 kΩ + 100 kΩ voltage divider; 12-bit ADC, 3.3 V reference.
        uint32_t raw = analogRead(PIN_BAT_ADC);
        return static_cast<uint32_t>(raw * 3300UL * 2 / 4095);
    }

    void deepSleep(uint64_t microseconds) override {
        esp_sleep_enable_timer_wakeup(microseconds);
        esp_deep_sleep_start();
    }

    const char *boardName() const override { return "FireBeetle2-ESP32E"; }

private:
    EpdDriver    _epd;
    Bme280Sensor _sensor;
};

// ─── Singleton accessor ───────────────────────────────────────────────────
IBoard &getBoard() {
    static Board board;
    return board;
}
