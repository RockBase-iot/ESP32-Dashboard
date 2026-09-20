#pragma once

// ─── Environmental sensor interface ────────────────────────────────────────
// Implement this interface for each on-board sensor (BME280, BME680, AHT20…).
// The BSP Board returns a pointer to its concrete implementation via
// IBoard::getTempSensor(); returns nullptr when no sensor is fitted.
//
// Power lifecycle — open / read / close:
//   Sensors behind a power-enable GPIO (AHT20 on the NM boards) draw hundreds
//   of microamps whenever they are powered, which is significant against a
//   sub-milliamp deep-sleep budget. They must therefore NOT be left powered
//   for the whole wake cycle:
//
//       sensor->begin();                          // power on
//       sensor->read(t, h, p);                    // sample
//       sensor->end();                            // power off immediately
//
//   begin() may be called again later to re-open. end() must be idempotent and
//   must leave the bus/pins in a non-leaking state. Callers should only ever
//   open the sensor when the data will actually be consumed, and must close it
//   before entering any sleep mode.
class ISensor {
public:
    virtual ~ISensor() = default;

    // Power on and initialize the sensor hardware. Returns true on success.
    // Calling begin() on an already-open sensor is a no-op returning true.
    virtual bool begin() = 0;

    // Read sensor values. Returns true if data is valid.
    // Returns false when the sensor is not currently open.
    virtual bool read(float &temp_c, float &humidity, float &pressure_hpa) = 0;

    // Power the sensor down and release its bus/pins. Must be idempotent, and
    // safe to call even if begin() was never called or failed.
    virtual void end() = 0;

    // Human-readable sensor type name, e.g. "BME280" or "BME680".
    virtual const char *typeName() const = 0;
};
