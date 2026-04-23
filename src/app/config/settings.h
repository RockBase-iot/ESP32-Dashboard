#pragma once

#include <Arduino.h>
#include <nvs_flash.h>

// Settings — thin wrapper around the ESP-IDF NVS API.
//
// Opens the given namespace in read-only or read-write mode.
// All Get* methods fall back to the supplied default value when the key is
// absent or the NVS handle could not be opened.
class Settings {
public:
    // Open the NVS namespace. Set read_write = true when storing values.
    Settings(const String &ns, bool read_write = false);
    ~Settings();

    // String accessors.
    String  GetString(const String &key, const String &default_value = "");
    void    SetString(const String &key, const String &value);

    // Signed 32-bit integer accessors.
    int32_t GetI32(const String &key, int32_t default_value = 0);
    void    SetI32(const String &key, int32_t value);

    // Boolean accessors (stored as uint8_t).
    bool    GetBool(const String &key, bool default_value = false);
    void    SetBool(const String &key, bool value);

    // Erase all keys in this namespace (use with caution).
    void    EraseAll();

    // Flush pending writes to flash. Returns true on success.
    bool    Commit();

private:
    nvs_handle_t _handle    = 0;
    bool         _read_write = false;
};
