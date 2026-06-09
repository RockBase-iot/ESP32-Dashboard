#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "drivers/sensor/aht20/Aht20Sensor.h"
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

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

        // Release GPIO hold that may have been set before the previous deep sleep,
        // then (re-)initialise each pin to its idle/off state.
        // Peripheral enable / control pins.
        gpio_hold_dis((gpio_num_t)PIN_PA_CTRL);
        gpio_hold_dis((gpio_num_t)PIN_LORA_EN);
        gpio_hold_dis((gpio_num_t)PIN_LORA_NSS);
        gpio_hold_dis((gpio_num_t)PIN_LORA_SCK);
        gpio_hold_dis((gpio_num_t)PIN_LORA_MOSI);
        gpio_hold_dis((gpio_num_t)PIN_LORA_MISO);
        gpio_hold_dis((gpio_num_t)PIN_LORA_RST);
        gpio_hold_dis((gpio_num_t)PIN_LORA_BUSY);
        gpio_hold_dis((gpio_num_t)PIN_LORA_DIO1);
        gpio_hold_dis((gpio_num_t)PIN_TF_CS);
        gpio_hold_dis((gpio_num_t)PIN_EPD_RST);
        gpio_hold_dis((gpio_num_t)PIN_TEMP_SDA);
        gpio_hold_dis((gpio_num_t)PIN_TEMP_SCL);
        gpio_hold_dis((gpio_num_t)PIN_CODEC_EN);
        gpio_hold_dis((gpio_num_t)PIN_ADC_EN);
        gpio_hold_dis((gpio_num_t)PIN_TEMP_CTL);

        // rev2: hardware enable pins — keep all modules powered off until needed.
        // ES8311 codec: hardware power cut via PIN_CODEC_EN; no I2C powerdown required.
        pinMode(PIN_LORA_EN,  OUTPUT); digitalWrite(PIN_LORA_EN,  LOW);  // LoRa off
        pinMode(PIN_CODEC_EN, OUTPUT); digitalWrite(PIN_CODEC_EN, LOW);  // Codec off
        pinMode(PIN_ADC_EN,   OUTPUT); digitalWrite(PIN_ADC_EN,   LOW);  // ADC off

        // Legacy control pins — idle/off states.
        pinMode(PIN_PA_CTRL,  OUTPUT); digitalWrite(PIN_PA_CTRL,  HIGH); // PA enabled (audio idle)
        pinMode(PIN_LORA_NSS,  INPUT); // LoRa SPI chip-select high-Z while module is powered off
        pinMode(PIN_LORA_SCK,  INPUT); // LoRa SPI clock high-Z while module is powered off
        pinMode(PIN_LORA_MOSI, INPUT); // LoRa SPI MOSI high-Z while module is powered off
        pinMode(PIN_LORA_MISO, INPUT); // LoRa SPI MISO high-Z while module is powered off
        pinMode(PIN_LORA_RST,  INPUT); // LoRa reset high-Z while module is powered off
        pinMode(PIN_LORA_BUSY, INPUT); // LoRa BUSY high-Z while module is powered off
        pinMode(PIN_LORA_DIO1, INPUT); // LoRa DIO1 high-Z while module is powered off
        // PIN_TEMP_CTL is driven HIGH by Aht20Sensor::begin() when the sensor is used.
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
        // rev2: gated resistor-divider network; enable ADC circuit, sample, then disable.
        pinMode(PIN_ADC_EN, OUTPUT);
        digitalWrite(PIN_ADC_EN, HIGH);
        delay(5); // allow divider network to settle
        uint32_t raw = analogRead(PIN_BATT_ADC); // IO3, ADC1_CH2
        digitalWrite(PIN_ADC_EN, LOW);
        // 12-bit ADC, 3.3 V reference, apply divider ratio.
        return static_cast<uint32_t>(raw * 3300UL * BATT_ADC_DIV / 4095);
    }

    void deepSleep(uint64_t microseconds) override {
        // ── Drive all power-enable pins LOW (modules off) ─────────────────────
        pinMode(PIN_LORA_EN,   OUTPUT); digitalWrite(PIN_LORA_EN,   LOW); // IO47 — LoRa power on (held LOW)
        pinMode(PIN_CODEC_EN,  OUTPUT); digitalWrite(PIN_CODEC_EN,  LOW);  // IO44 — ES8311 off
        pinMode(PIN_ADC_EN,    OUTPUT); digitalWrite(PIN_ADC_EN,    LOW);  // IO43 — ADC circuit off
        pinMode(PIN_TEMP_CTL,  OUTPUT); digitalWrite(PIN_TEMP_CTL,  LOW);  // IO40 — AHT20 off
        pinMode(PIN_PA_CTRL,   OUTPUT); digitalWrite(PIN_PA_CTRL,   LOW);  // IO41 — PA off
        pinMode(PIN_LORA_SCK,  OUTPUT); digitalWrite(PIN_LORA_SCK,  LOW);  // IO9  — LoRa/TF CLK
        pinMode(PIN_LORA_RST,  OUTPUT); digitalWrite(PIN_LORA_RST,  LOW);  // IO12 — LoRa RST
        pinMode(PIN_LORA_BUSY, OUTPUT); digitalWrite(PIN_LORA_BUSY, LOW);  // IO13 — LoRa BUSY
        pinMode(PIN_LORA_DIO1, OUTPUT); digitalWrite(PIN_LORA_DIO1, LOW);  // IO14 — LoRa DIO1

        // MOSI/MISO are RTC GPIOs (IO10/IO11). Drive them LOW through the RTC IO
        // mux so the level can be retained by the RTC domain during deep sleep.
        rtc_gpio_init((gpio_num_t)PIN_LORA_MOSI);                              // IO10 — LoRa MOSI
        rtc_gpio_set_direction((gpio_num_t)PIN_LORA_MOSI, RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level((gpio_num_t)PIN_LORA_MOSI, 0);
        rtc_gpio_init((gpio_num_t)PIN_LORA_MISO);                              // IO11 — LoRa MISO
        rtc_gpio_set_direction((gpio_num_t)PIN_LORA_MISO, RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level((gpio_num_t)PIN_LORA_MISO, 0);

        delay(10); // let all GPIO outputs stabilise

        // ── Set remaining GPIOs to high-Z (floating input, no pull) ──────────
        // IO0 (BOOT / EXT0 wakeup) is intentionally excluded — RTC GPIO must not
        // be reconfigured here; the wakeup subsystem owns it.
        static const uint8_t kHiZ[] = {
            PIN_EPD_BUSY, PIN_EPD_CS,  PIN_EPD_DC,  PIN_EPD_SCK,  PIN_EPD_MOSI,  PIN_EPD_RST, // EPD SPI

            PIN_LORA_NSS, 

            PIN_TF_CS,                                                             // TF CS
            PIN_TEMP_SDA, PIN_TEMP_SCL,                                            // I2C bus
            PIN_I2S_SCLK, PIN_I2S_ASDOUT, PIN_I2S_LRCK, PIN_I2S_DSIN, PIN_I2S_MCLK, // I2S
            PIN_BATT_ADC,
            PIN_AP_BTN,
        };
        for (uint8_t p : kHiZ) {
            pinMode(p, INPUT); // INPUT = floating, no pull-up/pull-down
        }

        // ── Latch driven output pins across deep sleep ────────────────────────
        // Latched LOW:
        gpio_hold_en((gpio_num_t)PIN_CODEC_EN);
        gpio_hold_en((gpio_num_t)PIN_ADC_EN);
        gpio_hold_en((gpio_num_t)PIN_TEMP_CTL);
        gpio_hold_en((gpio_num_t)PIN_PA_CTRL);
        gpio_hold_en((gpio_num_t)PIN_LORA_SCK);
        gpio_hold_en((gpio_num_t)PIN_LORA_RST);
        gpio_hold_en((gpio_num_t)PIN_LORA_BUSY);
        gpio_hold_en((gpio_num_t)PIN_LORA_DIO1);
        // MOSI/MISO are RTC GPIOs (IO10/IO11) — use rtc_gpio_hold_en() so the
        // LOW output level is retained by the RTC IO domain during deep sleep.
        // (gpio_hold_en() does not reliably latch RTC pads across deep sleep.)
        rtc_gpio_hold_en((gpio_num_t)PIN_LORA_MOSI);
        rtc_gpio_hold_en((gpio_num_t)PIN_LORA_MISO);
        // Latched HIGH:
        gpio_hold_en((gpio_num_t)PIN_LORA_EN);
        gpio_deep_sleep_hold_en(); // ESP32-S3: retain latches when IO domain powers off

        Serial.flush(); // drain USB CDC TX buffer before digital core powers off

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
