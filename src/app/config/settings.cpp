#include "settings.h"

#include <nvs.h>
#include "utils/logger.h"

static const char *TAG = "Settings";

Settings::Settings(const String &ns, bool read_write) : _read_write(read_write) {
    nvs_flash_init();
    nvs_open(ns.c_str(),
             read_write ? NVS_READWRITE : NVS_READONLY,
             &_handle);
}

Settings::~Settings() {
    if (_handle) nvs_close(_handle);
}

String Settings::GetString(const String &key, const String &default_value) {
    if (!_handle) return default_value;
    size_t len = 0;
    if (nvs_get_str(_handle, key.c_str(), nullptr, &len) != ESP_OK) {
        return default_value;
    }
    char *buf = new char[len];
    nvs_get_str(_handle, key.c_str(), buf, &len);
    String result(buf);
    delete[] buf;
    return result;
}

void Settings::SetString(const String &key, const String &value) {
    if (!_handle || !_read_write) return;
    nvs_set_str(_handle, key.c_str(), value.c_str());
}

int32_t Settings::GetI32(const String &key, int32_t default_value) {
    if (!_handle) return default_value;
    int32_t val = default_value;
    nvs_get_i32(_handle, key.c_str(), &val);
    return val;
}

void Settings::SetI32(const String &key, int32_t value) {
    if (!_handle || !_read_write) return;
    nvs_set_i32(_handle, key.c_str(), value);
}

bool Settings::GetBool(const String &key, bool default_value) {
    if (!_handle) return default_value;
    uint8_t val = default_value ? 1 : 0;
    nvs_get_u8(_handle, key.c_str(), &val);
    return val != 0;
}

void Settings::SetBool(const String &key, bool value) {
    if (!_handle || !_read_write) return;
    nvs_set_u8(_handle, key.c_str(), value ? 1 : 0);
}

void Settings::EraseAll() {
    if (!_handle || !_read_write) return;
    nvs_erase_all(_handle);
}

bool Settings::Commit() {
    if (!_handle || !_read_write) return false;
    return nvs_commit(_handle) == ESP_OK;
}
