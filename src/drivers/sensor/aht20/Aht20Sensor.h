#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#include "drivers/sensor/ISensor.h"

// ─── AHT20 sensor driver ───────────────────────────────────────────────────
// Wraps Adafruit_AHTX0 and implements ISensor.
// The ctlPin is a GPIO that must be driven HIGH before the sensor is powered.
// Pass -1 for ctlPin if no power-enable pin is used.
class Aht20Sensor final : public ISensor {
public:
    // sdaPin / sclPin: I2C bus pins.
    // ctlPin: power-enable GPIO (driven HIGH in begin()); -1 = none.
    explicit Aht20Sensor(int sdaPin, int sclPin, int ctlPin = -1)
        : _sdaPin(sdaPin), _sclPin(sclPin), _ctlPin(ctlPin) {}

    bool begin() override {
        if (_ctlPin >= 0) {
            pinMode(_ctlPin, OUTPUT);
            digitalWrite(_ctlPin, HIGH);
            delay(200); // allow sensor power rail and internal state to settle
        }
        _wire.begin(_sdaPin, _sclPin);
        _wire.setClock(100000);

        // AHT20 may NACK briefly right after power-up; retry init a few times.
        for (uint8_t attempt = 0; attempt < 5; ++attempt) {
            if (_aht.begin(&_wire)) {
                return true;
            }
            delay(40);
        }
        return false;
    }

    bool read(float &temp_c, float &humidity, float &pressure_hpa) override {
        sensors_event_t hEvt, tEvt;
        if (!_aht.getEvent(&hEvt, &tEvt)) {
            return false;
        }
        temp_c       = tEvt.temperature;
        humidity     = hEvt.relative_humidity;
        pressure_hpa = 0.0f; // AHT20 has no barometer
        return true;
    }

    const char *typeName() const override { return "AHT20"; }

private:
    int           _sdaPin;
    int           _sclPin;
    int           _ctlPin;
    TwoWire       _wire{1};   // Use I2C bus 1 to avoid conflicts
    Adafruit_AHTX0 _aht;
};
