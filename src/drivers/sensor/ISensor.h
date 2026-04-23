#pragma once

// ─── Environmental sensor interface ────────────────────────────────────────
// Implement this interface for each on-board sensor (BME280, BME680, AHT20…).
// The BSP Board returns a pointer to its concrete implementation via
// IBoard::sensor(); returns nullptr when no sensor is fitted.
class ISensor {
public:
    virtual ~ISensor() = default;

    // Initialize the sensor hardware. Returns true on success.
    virtual bool begin() = 0;

    // Read sensor values. Returns true if data is valid.
    virtual bool read(float &temp_c, float &humidity, float &pressure_hpa) = 0;

    // Human-readable sensor type name, e.g. "BME280" or "BME680".
    virtual const char *typeName() const = 0;
};
