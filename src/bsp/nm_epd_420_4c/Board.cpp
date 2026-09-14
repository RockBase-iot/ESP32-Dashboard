#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_4C.h>
#include <epd4c/GxEPD2_420c_GDEY0420F51.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <soc/gpio_struct.h>

#include "bsp/IBoard.h"
#include "drivers/sensor/aht20/Aht20Sensor.h"
#include "config.h"
#include "utils/logger.h"

namespace {
using EpdDriverType = GxEPD2_420c_GDEY0420F51;
using DisplayType = GxEPD2_4C<EpdDriverType, EpdDriverType::HEIGHT>;

EpdDriverType epdDriver(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY);
DisplayType display(epdDriver);
volatile uint64_t lightSleepIsrStatus = 0;

void IRAM_ATTR lightSleepWakeIsr(void *arg) {
    const uint8_t pin = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(arg));
    if (pin < 64) {
        lightSleepIsrStatus |= 1ULL << pin;
    }
}

class EpdDriver final : public IEpdDriver {
public:
    void init(bool initialPowerOn) override {
        SPI.begin(PIN_EPD_SCK, PIN_EPD_MISO, PIN_EPD_MOSI, PIN_EPD_CS);
        display.init(115200, initialPowerOn, 2, false);
    }

    void hibernate() override { display.hibernate(); }
    void firstPage() override { display.firstPage(); }
    bool nextPage() override { return display.nextPage(); }
};

void setOutput(uint8_t pin, uint8_t level) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, level);
}

void removeLightSleepWakeHandlers() {
    gpio_wakeup_disable(static_cast<gpio_num_t>(PIN_BOOT_BTN));
    gpio_wakeup_disable(static_cast<gpio_num_t>(PIN_USER_BTN));
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    gpio_isr_handler_remove(static_cast<gpio_num_t>(PIN_BOOT_BTN));
    gpio_isr_handler_remove(static_cast<gpio_num_t>(PIN_USER_BTN));
    gpio_set_intr_type(static_cast<gpio_num_t>(PIN_BOOT_BTN), GPIO_INTR_DISABLE);
    gpio_set_intr_type(static_cast<gpio_num_t>(PIN_USER_BTN), GPIO_INTR_DISABLE);
}
}  // namespace

class Board final : public IBoard {
public:
    void init() override {
        Serial.begin(115200);
        gpio_deep_sleep_hold_dis();
        const uint8_t heldPins[] = {PIN_PA_CTRL, PIN_LORA_EN, PIN_CODEC_EN, PIN_ADC_EN,
                                    PIN_TEMP_CTL, PIN_EPD_RST};
        for (uint8_t pin : heldPins) {
            gpio_hold_dis(static_cast<gpio_num_t>(pin));
        }

        setOutput(PIN_LORA_EN, LOW);
        setOutput(PIN_CODEC_EN, LOW);
        setOutput(PIN_ADC_EN, LOW);
        setOutput(PIN_PA_CTRL, HIGH);
        setOutput(PIN_TEMP_CTL, LOW);

        const uint8_t hiZPins[] = {
            PIN_LORA_NSS, PIN_LORA_SCK, PIN_LORA_MOSI, PIN_LORA_MISO, PIN_LORA_RST,
            PIN_LORA_BUSY, PIN_LORA_DIO1, PIN_EPD_BUSY, PIN_EPD_CS, PIN_EPD_DC,
            PIN_EPD_SCK, PIN_EPD_MOSI, PIN_EPD_RST, PIN_TEMP_SCL, PIN_TEMP_SDA,
            PIN_TF_CS, PIN_I2S_SCLK, PIN_I2S_ASDOUT, PIN_I2S_LRCK, PIN_I2S_DSIN,
            PIN_I2S_MCLK, PIN_BATT_ADC,
        };
        for (uint8_t pin : hiZPins) {
            pinMode(pin, INPUT);
        }
        pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
        pinMode(PIN_USER_BTN, INPUT_PULLUP);
    }

    IEpdDriver &epd() override { return epdAdapter; }
    Adafruit_GFX &gfx() override { return display; }
    uint16_t dispWidth() const override { return DISP_WIDTH; }
    uint16_t dispHeight() const override { return DISP_HEIGHT; }
    uint16_t colorBlack() const override { return GxEPD_BLACK; }
    uint16_t colorWhite() const override { return GxEPD_WHITE; }
    uint16_t colorAccent() const override { return GxEPD_RED; }
    bool hasAccentColor() const override { return true; }
    ISensor *getTempSensor() override { return &sensor; }

    uint32_t readBatteryMv() override {
        setOutput(PIN_ADC_EN, HIGH);
        delay(5);
        const uint32_t raw = analogRead(PIN_BATT_ADC);
        setOutput(PIN_ADC_EN, LOW);
        return raw * 3300UL * BATT_ADC_DIV / 4095UL;
    }

    void prepareForSleep() override {
        setOutput(PIN_LORA_EN, LOW);
        setOutput(PIN_CODEC_EN, LOW);
        setOutput(PIN_ADC_EN, LOW);
        setOutput(PIN_TEMP_CTL, LOW);
        setOutput(PIN_PA_CTRL, LOW);
        const uint8_t pins[] = {
            PIN_LORA_NSS, PIN_LORA_SCK, PIN_LORA_MOSI, PIN_LORA_MISO, PIN_LORA_RST,
            PIN_LORA_BUSY, PIN_LORA_DIO1, PIN_EPD_BUSY, PIN_EPD_CS, PIN_EPD_DC,
            PIN_EPD_SCK, PIN_EPD_MOSI, PIN_EPD_RST, PIN_TEMP_SCL, PIN_TEMP_SDA,
            PIN_TF_CS, PIN_I2S_SCLK, PIN_I2S_ASDOUT, PIN_I2S_LRCK, PIN_I2S_DSIN,
            PIN_I2S_MCLK, PIN_BATT_ADC, PIN_USER_BTN,
        };
        for (uint8_t pin : pins) {
            pinMode(pin, INPUT);
        }
    }

    void deepSleep(uint64_t microseconds) override {
        prepareForSleep();
        pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
        gpio_hold_en(static_cast<gpio_num_t>(PIN_LORA_EN));
        gpio_hold_en(static_cast<gpio_num_t>(PIN_CODEC_EN));
        gpio_hold_en(static_cast<gpio_num_t>(PIN_ADC_EN));
        gpio_hold_en(static_cast<gpio_num_t>(PIN_TEMP_CTL));
        gpio_hold_en(static_cast<gpio_num_t>(PIN_PA_CTRL));
        gpio_deep_sleep_hold_en();
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        if (microseconds > 0) {
            esp_sleep_enable_timer_wakeup(microseconds);
        }
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
        Serial.flush();
        esp_deep_sleep_start();
    }

    LightWake lightSleepMs(uint32_t maxMs) override {
        pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
        pinMode(PIN_USER_BTN, INPUT_PULLUP);
        lightSleepIsrStatus = 0;
        const esp_err_t isrService = gpio_install_isr_service(0);
        gpio_set_intr_type(static_cast<gpio_num_t>(PIN_BOOT_BTN), GPIO_INTR_LOW_LEVEL);
        gpio_set_intr_type(static_cast<gpio_num_t>(PIN_USER_BTN), GPIO_INTR_LOW_LEVEL);
        const esp_err_t bootIsr = gpio_isr_handler_add(
            static_cast<gpio_num_t>(PIN_BOOT_BTN), lightSleepWakeIsr,
            reinterpret_cast<void *>(static_cast<uintptr_t>(PIN_BOOT_BTN)));
        const esp_err_t userIsr = gpio_isr_handler_add(
            static_cast<gpio_num_t>(PIN_USER_BTN), lightSleepWakeIsr,
            reinterpret_cast<void *>(static_cast<uintptr_t>(PIN_USER_BTN)));
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        const esp_err_t gpioWake = esp_sleep_enable_gpio_wakeup();
        const esp_err_t bootWake = gpio_wakeup_enable(
            static_cast<gpio_num_t>(PIN_BOOT_BTN), GPIO_INTR_LOW_LEVEL);
        const esp_err_t userWake = gpio_wakeup_enable(
            static_cast<gpio_num_t>(PIN_USER_BTN), GPIO_INTR_LOW_LEVEL);
        const esp_err_t timerWake = maxMs == 0
            ? ESP_OK
            : esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(maxMs) * 1000ULL);
        const bool setupOk =
            (isrService == ESP_OK || isrService == ESP_ERR_INVALID_STATE) &&
            bootIsr == ESP_OK && userIsr == ESP_OK && gpioWake == ESP_OK &&
            bootWake == ESP_OK && userWake == ESP_OK && timerWake == ESP_OK;
        if (!setupOk) {
            removeLightSleepWakeHandlers();
            return LightWake::Other;
        }

        Serial.flush();
        esp_light_sleep_start();
        const uint64_t gpioStatus = static_cast<uint64_t>(GPIO.status) |
                                     (static_cast<uint64_t>(GPIO.status1.val) << 32);
        const uint64_t wakeStatus = gpioStatus | lightSleepIsrStatus;
        const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        removeLightSleepWakeHandlers();
        return classifyLightWake(
            cause == ESP_SLEEP_WAKEUP_TIMER,
            cause == ESP_SLEEP_WAKEUP_GPIO,
            wakeStatus,
            PIN_BOOT_BTN,
            PIN_USER_BTN,
            digitalRead(PIN_BOOT_BTN) == LOW,
            digitalRead(PIN_USER_BTN) == LOW);
    }

    LightWake timerOnlyLightSleepMs(uint32_t maxMs) override {
        pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
        pinMode(PIN_USER_BTN, INPUT_PULLUP);
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        if (maxMs > 0 && esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(maxMs) * 1000ULL) != ESP_OK) {
            return LightWake::Other;
        }
        Serial.flush();
        esp_light_sleep_start();
        const bool timer = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        return timer ? LightWake::Timeout : LightWake::Other;
    }

    uint8_t bootButtonPin() const override { return PIN_BOOT_BTN; }
    uint8_t apButtonPin() const override { return PIN_USER_BTN; }
    const char *boardName() const override { return "NM-EPD-420-4C"; }

private:
    EpdDriver epdAdapter;
    Aht20Sensor sensor{PIN_TEMP_SDA, PIN_TEMP_SCL, PIN_TEMP_CTL};
};

IBoard &getBoard() {
    static Board board;
    return board;
}
