#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_3C.h>
#include "drivers/sensor/aht20/Aht20Sensor.h"
#include <esp_sleep.h>
#include <esp_wifi.h>
#include <driver/gpio.h>

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

        // Release GPIO hold that may have been set before the previous deep sleep,
        // then (re-)initialise each pin to its idle/off state.
        // EPD SPI output pins — released before SPI.begin() takes ownership.
        gpio_hold_dis((gpio_num_t)PIN_EPD_CS);
        gpio_hold_dis((gpio_num_t)PIN_EPD_RST);
        gpio_hold_dis((gpio_num_t)PIN_EPD_DC);
        gpio_hold_dis((gpio_num_t)PIN_EPD_MOSI);
        gpio_hold_dis((gpio_num_t)PIN_EPD_SCK);
        // Peripheral enable / control pins.
        gpio_hold_dis((gpio_num_t)PIN_PA_CTRL);
        gpio_hold_dis((gpio_num_t)PIN_LORA_EN);
        gpio_hold_dis((gpio_num_t)PIN_CODEC_EN);
        gpio_hold_dis((gpio_num_t)PIN_ADC_EN);
        gpio_hold_dis((gpio_num_t)PIN_TEMP_CTL);
        gpio_hold_dis((gpio_num_t)PIN_LORA_RST);
        gpio_hold_dis((gpio_num_t)PIN_LORA_NSS);

        // rev2: hardware enable pins — keep all modules powered off until needed.
        // ES8311 codec: hardware power cut via PIN_CODEC_EN; no I2C powerdown required.
        pinMode(PIN_LORA_EN,  OUTPUT); digitalWrite(PIN_LORA_EN,  LOW);  // LoRa off
        pinMode(PIN_CODEC_EN, OUTPUT); digitalWrite(PIN_CODEC_EN, LOW);  // Codec off
        pinMode(PIN_ADC_EN,   OUTPUT); digitalWrite(PIN_ADC_EN,   LOW);  // ADC off

        // Legacy control pins — idle/off states.
        pinMode(PIN_PA_CTRL,  OUTPUT); digitalWrite(PIN_PA_CTRL,  HIGH); // PA enabled (audio idle)
        pinMode(PIN_LORA_RST, OUTPUT); digitalWrite(PIN_LORA_RST, HIGH); // LoRa not in reset
        pinMode(PIN_LORA_NSS, OUTPUT); digitalWrite(PIN_LORA_NSS, HIGH); // LoRa SPI deselected
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
        // ── Step 1: Stop WiFi modem ──────────────────────────────────────────
        // Safety net regardless of earlier wifi.disconnect() calls.
        // An active modem power domain alone consumes ~35 mA in deep sleep.
        esp_wifi_stop();

        // ── Step 2: Release SPI bus, then hold EPD SPI pins at safe levels ──
        // After _display.hibernate(), the SPI peripheral is still initialised.
        // When the digital core powers off in deep sleep the SPI peripheral loses
        // power and all SPI output pins float.  If CS floats LOW the EPD
        // controller is "selected" and may exit hibernate; if RST floats LOW the
        // controller resets entirely — both states increase EPD current.
        SPI.end(); // detaches pins from SPI peripheral → regular GPIO inputs
        // CS HIGH = deselected; RST HIGH = controller stays in hibernate state.
        pinMode(PIN_EPD_CS,   OUTPUT); digitalWrite(PIN_EPD_CS,   HIGH);
        pinMode(PIN_EPD_RST,  OUTPUT); digitalWrite(PIN_EPD_RST,  HIGH);
        pinMode(PIN_EPD_DC,   OUTPUT); digitalWrite(PIN_EPD_DC,   LOW);
        pinMode(PIN_EPD_MOSI, OUTPUT); digitalWrite(PIN_EPD_MOSI, LOW);
        pinMode(PIN_EPD_SCK,  OUTPUT); digitalWrite(PIN_EPD_SCK,  LOW);
        // PIN_EPD_BUSY is driven by the EPD panel — leave as input, do not hold.

        // ── Step 3: Power off all external module enable pins ────────────────
        // rev2 hardware enable pins:
        pinMode(PIN_LORA_EN,  OUTPUT); digitalWrite(PIN_LORA_EN,  LOW);  // LoRa off
        pinMode(PIN_CODEC_EN, OUTPUT); digitalWrite(PIN_CODEC_EN, LOW);  // ES8311 off
        pinMode(PIN_ADC_EN,   OUTPUT); digitalWrite(PIN_ADC_EN,   LOW);  // ADC circuit off
        pinMode(PIN_TEMP_CTL, OUTPUT); digitalWrite(PIN_TEMP_CTL, LOW);  // AHT20 off
        // Legacy control pins:
        pinMode(PIN_PA_CTRL,  OUTPUT); digitalWrite(PIN_PA_CTRL,  LOW);  // PA off
        pinMode(PIN_LORA_RST, OUTPUT); digitalWrite(PIN_LORA_RST, LOW);  // LoRa in reset
        pinMode(PIN_LORA_NSS, OUTPUT); digitalWrite(PIN_LORA_NSS, HIGH); // LoRa SPI CS idle
        delay(10); // let all GPIO outputs stabilise

        // ── Step 4: Latch every driven pin across deep sleep ─────────────────
        gpio_hold_en((gpio_num_t)PIN_EPD_CS);
        gpio_hold_en((gpio_num_t)PIN_EPD_RST);
        gpio_hold_en((gpio_num_t)PIN_EPD_DC);
        gpio_hold_en((gpio_num_t)PIN_EPD_MOSI);
        gpio_hold_en((gpio_num_t)PIN_EPD_SCK);
        gpio_hold_en((gpio_num_t)PIN_LORA_EN);
        gpio_hold_en((gpio_num_t)PIN_CODEC_EN);
        gpio_hold_en((gpio_num_t)PIN_ADC_EN);
        gpio_hold_en((gpio_num_t)PIN_TEMP_CTL);
        gpio_hold_en((gpio_num_t)PIN_PA_CTRL);
        gpio_hold_en((gpio_num_t)PIN_LORA_RST);
        gpio_hold_en((gpio_num_t)PIN_LORA_NSS);
        gpio_deep_sleep_hold_en(); // ESP32-S3: retain latches when IO domain powers off

        // ── Step 5: Flush Serial (USB CDC on ESP32-S3) then sleep ────────────
        // Without flush the USB PHY may still be transmitting when the core
        // powers off, delaying modem-domain shutdown and adding transient current.
        Serial.flush();

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
