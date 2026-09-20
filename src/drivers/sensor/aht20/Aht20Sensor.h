#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#include "drivers/sensor/ISensor.h"
#include "utils/logger.h"

// ─── AHT20 sensor driver ───────────────────────────────────────────────────
// Wraps Adafruit_AHTX0 and implements ISensor.
// The ctlPin is a GPIO that must be driven HIGH before the sensor is powered.
// Pass -1 for ctlPin if no power-enable pin is used.
//
// The AHT20 is only powered while a reading is being taken: begin() raises the
// power-enable pin, read() samples, and end() drops the pin again and releases
// the I2C bus. Leaving the pin HIGH for the whole wake cycle would add roughly
// 0.25–1 mA, which is comparable to the entire deep-sleep budget.
class Aht20Sensor final : public ISensor {
public:
    // sdaPin / sclPin: I2C bus pins.
    // ctlPin: power-enable GPIO (driven HIGH in begin()); -1 = none.
    explicit Aht20Sensor(int sdaPin, int sclPin, int ctlPin = -1)
        : _sdaPin(sdaPin), _sclPin(sclPin), _ctlPin(ctlPin) {}

    bool begin() override {
        if (_active) {
            return true; // already powered on
        }

        _wire.begin(_sdaPin, _sclPin);
        _wire.setClock(100000);

        if (_ctlPin >= 0) {
            pinMode(_ctlPin, OUTPUT);
            digitalWrite(_ctlPin, HIGH);
            // The AHT20 has an internal power-on-reset of a few ms before it
            // starts answering; probing immediately would just NACK.
            delay(kPowerUpDelayMs);
        }

        // Probe the bus before handing over to the driver. Adafruit_AHTX0::
        // begin() has an unbounded `while (getStatus() & BUSY)` loop, and
        // getStatus() returns 0xFF on a read failure — 0xFF & 0x80 is truthy,
        // so a missing or unpowered sensor makes begin() spin forever instead
        // of returning false. Confirming the address answers first keeps a dead
        // sensor from hanging the wake cycle.
        //
        // This probe is what makes the no-sensor case cheap. Indoor Climate is
        // enabled by default, so every board without an AHT20 reaches here on
        // each weather render; failing in one I2C timeout (a few ms) instead of
        // burning the vendor driver's full init budget keeps that acceptable.
        if (!_probe()) {
            log_e("AHT20", "no ACK at 0x38 on SDA=%d SCL=%d (check PIN_TEMP_CTL / wiring)",
                  _sdaPin, _sclPin);
            end();
            return false;
        }

        // AHT20 may NACK briefly right after power-up; retry init a few times.
        for (uint8_t attempt = 0; attempt < 5; ++attempt) {
            if (_aht.begin(&_wire)) {
                _active = true;
                return true;
            }
            delay(40);
        }

        // Init failed — power straight back down so a dead sensor cannot keep
        // draining the battery for the rest of the wake cycle.
        log_e("AHT20", "present at 0x38 but calibration did not complete");
        end();
        return false;
    }

    bool read(float &temp_c, float &humidity, float &pressure_hpa) override {
        if (!_active) {
            return false; // not powered — caller must begin() first
        }
        sensors_event_t hEvt, tEvt;
        if (!_aht.getEvent(&hEvt, &tEvt)) {
            return false;
        }
        temp_c       = tEvt.temperature;
        humidity     = hEvt.relative_humidity;
        pressure_hpa = 0.0f; // AHT20 has no barometer
        return true;
    }

    void end() override {
        _wire.end(); // release I2C peripheral from SDA/SCL first
        if (_ctlPin >= 0) {
            // Drive the enable pin LOW and keep it as a driven output so it
            // cannot float and back-feed the sensor rail through its pull-ups.
            pinMode(_ctlPin, OUTPUT);
            digitalWrite(_ctlPin, LOW);
        }
        _active = false;
    }

    const char *typeName() const override { return "AHT20"; }

private:
    // AHT20 power-on-reset time before the part answers on I2C. The previous
    // 200 ms was a conservative settle for the measurement path; the probe only
    // needs the bus to come alive, and the vendor driver's own retry loop below
    // covers the rest. A sensor that does answer still waits for its full
    // calibration inside Adafruit_AHTX0::begin().
    static constexpr uint16_t kPowerUpDelayMs = 40;

    // True when a device ACKs at the AHT20 address. Used to fail fast instead
    // of letting the vendor driver's unbounded busy-wait loop hang the wake.
    bool _probe() {
        _wire.beginTransmission(_address);
        return _wire.endTransmission() == 0;
    }

    int            _sdaPin;
    int            _sclPin;
    int            _ctlPin;
    bool           _active = false;
    uint8_t        _address = 0x38;  // AHT20 default I2C address
    TwoWire        _wire{1};   // Use I2C bus 1 to avoid conflicts
    Adafruit_AHTX0 _aht;
};
