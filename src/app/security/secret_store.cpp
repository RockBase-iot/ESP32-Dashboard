#include "secret_store.h"

#include <cstdio>
#include <cstdlib>

#include <nvs.h>
#include <nvs_flash.h>

#include "utils/logger.h"

static const char *TAG_CAL_SECRETS = "CalendarSecrets";

namespace {
std::string indexedKey(const char *prefix, uint8_t index) {
    char buffer[12];
    std::snprintf(buffer, sizeof(buffer), "%s%02u", prefix, static_cast<unsigned>(index));
    return std::string(buffer);
}

std::string getOrEmpty(const ISecretBackend &backend, const std::string &key) {
    std::string value;
    backend.getString(key, value);
    return value;
}

bool parseBool(const std::string &value) {
    return value == "1" || value == "true";
}

uint8_t parseColor(const std::string &value) {
    if (value.empty()) {
        return 0;
    }
    const int parsed = std::atoi(value.c_str());
    if (parsed < 0) {
        return 0;
    }
    if (parsed > 255) {
        return 255;
    }
    return static_cast<uint8_t>(parsed);
}

bool removeAllSourceKeys(ISecretBackend &backend, uint8_t index) {
    bool ok = true;
    ok = backend.remove(calendarSourceUrlKey(index)) && ok;
    ok = backend.remove(calendarSourceApiKeyKey(index)) && ok;
    ok = backend.remove(calendarSourceAliasKey(index)) && ok;
    ok = backend.remove(calendarSourceEnabledKey(index)) && ok;
    ok = backend.remove(calendarSourceColorKey(index)) && ok;
    ok = backend.remove(calendarSourceEtagKey(index)) && ok;
    ok = backend.remove(calendarSourceLastModifiedKey(index)) && ok;
    ok = backend.remove(calendarSourceErrorKey(index)) && ok;
    ok = backend.remove(calendarSourceCacheMarkerKey(index)) && ok;
    return ok;
}
}  // namespace

bool MemorySecretBackend::getString(const std::string &key, std::string &out) const {
    const auto it = _values.find(key);
    if (it == _values.end()) {
        return false;
    }
    out = it->second;
    return true;
}

bool MemorySecretBackend::setString(const std::string &key, const std::string &value) {
    _values[key] = value;
    return true;
}

bool MemorySecretBackend::remove(const std::string &key) {
    _values.erase(key);
    return true;
}

bool MemorySecretBackend::commit() {
    return true;
}

bool MemorySecretBackend::hasKey(const std::string &key) const {
    return _values.find(key) != _values.end();
}

NvsSecretBackend::NvsSecretBackend(const char *nameSpace) {
    nvs_flash_init();
    nvs_handle_t handle = 0;
    if (nvs_open(nameSpace, NVS_READWRITE, &handle) == ESP_OK) {
        _handle = reinterpret_cast<void *>(handle);
    }
}

NvsSecretBackend::~NvsSecretBackend() {
    if (_handle) {
        nvs_close(static_cast<nvs_handle_t>(reinterpret_cast<uintptr_t>(_handle)));
    }
}

bool NvsSecretBackend::getString(const std::string &key, std::string &out) const {
    if (!_handle) {
        return false;
    }
    const nvs_handle_t handle = static_cast<nvs_handle_t>(reinterpret_cast<uintptr_t>(_handle));
    size_t len = 0;
    if (nvs_get_str(handle, key.c_str(), nullptr, &len) != ESP_OK || len == 0) {
        return false;
    }
    std::string value;
    value.resize(len);
    if (nvs_get_str(handle, key.c_str(), &value[0], &len) != ESP_OK) {
        return false;
    }
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    out = value;
    return true;
}

bool NvsSecretBackend::setString(const std::string &key, const std::string &value) {
    if (!_handle) {
        return false;
    }
    const nvs_handle_t handle = static_cast<nvs_handle_t>(reinterpret_cast<uintptr_t>(_handle));
    return nvs_set_str(handle, key.c_str(), value.c_str()) == ESP_OK;
}

bool NvsSecretBackend::remove(const std::string &key) {
    if (!_handle) {
        return false;
    }
    const nvs_handle_t handle = static_cast<nvs_handle_t>(reinterpret_cast<uintptr_t>(_handle));
    const esp_err_t err = nvs_erase_key(handle, key.c_str());
    return err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND;
}

bool NvsSecretBackend::commit() {
    if (!_handle) {
        return false;
    }
    const nvs_handle_t handle = static_cast<nvs_handle_t>(reinterpret_cast<uintptr_t>(_handle));
    return nvs_commit(handle) == ESP_OK;
}

CalendarSecretStore::CalendarSecretStore(ISecretBackend &backend) : _backend(backend) {}

bool CalendarSecretStore::saveSource(const CalendarSourceSecrets &source) {
    if (!isValidCalendarSourceIndex(source.index)) {
        log_w(TAG_CAL_SECRETS, "Save rejected: invalid calendar slot=%u",
              static_cast<unsigned>(source.index));
        return false;
    }
    const auto normalized = normalizeCalendarSourceUrl(source.url);
    if (!normalized.ok) {
        log_w(TAG_CAL_SECRETS, "Save rejected: slot=%u urlLen=%u policy=%u",
              static_cast<unsigned>(source.index),
              static_cast<unsigned>(source.url.size()),
              static_cast<unsigned>(normalized.status));
        return false;
    }
    bool ok = true;
    ok = _backend.setString(calendarSourceUrlKey(source.index), normalized.normalizedUrl) && ok;
    ok = _backend.setString(calendarSourceApiKeyKey(source.index), source.apiKey) && ok;
    ok = _backend.setString(calendarSourceAliasKey(source.index), source.alias) && ok;
    ok = _backend.setString(calendarSourceEnabledKey(source.index), source.enabled ? "1" : "0") && ok;
    ok = _backend.setString(calendarSourceColorKey(source.index), std::to_string(source.color)) && ok;
    const bool committed = _backend.commit();
    log_i(TAG_CAL_SECRETS,
          "Saved source slot=%u enabled=%d host=%s label=%s urlHash=0x%08lx urlLen=%u alias=%d apiKey=%d color=%u writes=%d commit=%d",
          static_cast<unsigned>(source.index), source.enabled ? 1 : 0,
          normalized.host.c_str(),
          calendarSourceDiagnosticLabel(normalized.normalizedUrl).c_str(),
          static_cast<unsigned long>(calendarSourceDiagnosticHash(normalized.normalizedUrl)),
          static_cast<unsigned>(normalized.normalizedUrl.size()),
          source.alias.empty() ? 0 : 1, source.apiKey.empty() ? 0 : 1,
          static_cast<unsigned>(source.color), ok ? 1 : 0, committed ? 1 : 0);
    return committed && ok;
}

CalendarSourceMetadata CalendarSecretStore::readMetadata(uint8_t index) const {
    CalendarSourceMetadata meta;
    meta.index = index;
    if (!isValidCalendarSourceIndex(index)) {
        return meta;
    }
    const std::string url = getOrEmpty(_backend, calendarSourceUrlKey(index));
    if (url.empty()) {
        return meta;
    }
    const auto normalized = normalizeCalendarSourceUrl(url);
    meta.configured = normalized.ok;
    meta.host = normalized.host;
    meta.maskedUrl = maskCalendarUrlForDisplay(normalized.normalizedUrl, normalized.host);
    meta.maskedApiKey = maskCalendarSecret(getOrEmpty(_backend, calendarSourceApiKeyKey(index)));
    meta.alias = getOrEmpty(_backend, calendarSourceAliasKey(index));
    meta.enabled = parseBool(getOrEmpty(_backend, calendarSourceEnabledKey(index)));
    meta.color = parseColor(getOrEmpty(_backend, calendarSourceColorKey(index)));
    return meta;
}

bool CalendarSecretStore::loadSourceForDownload(uint8_t index, CalendarSourceSecrets &out) const {
    if (!isValidCalendarSourceIndex(index)) {
        log_w(TAG_CAL_SECRETS, "Load rejected: invalid calendar slot=%u",
              static_cast<unsigned>(index));
        return false;
    }
    const std::string url = getOrEmpty(_backend, calendarSourceUrlKey(index));
    if (url.empty()) {
        log_d(TAG_CAL_SECRETS, "Load skipped: slot=%u no stored URL",
              static_cast<unsigned>(index));
        return false;
    }
    const auto normalized = normalizeCalendarSourceUrl(url);
    if (!normalized.ok) {
        log_w(TAG_CAL_SECRETS, "Load rejected: slot=%u storedUrlLen=%u policy=%u",
              static_cast<unsigned>(index), static_cast<unsigned>(url.size()),
              static_cast<unsigned>(normalized.status));
        return false;
    }
    out.index = index;
    out.url = normalized.normalizedUrl;
    out.apiKey = getOrEmpty(_backend, calendarSourceApiKeyKey(index));
    out.alias = getOrEmpty(_backend, calendarSourceAliasKey(index));
    out.enabled = parseBool(getOrEmpty(_backend, calendarSourceEnabledKey(index)));
    out.color = parseColor(getOrEmpty(_backend, calendarSourceColorKey(index)));
    log_i(TAG_CAL_SECRETS,
          "Loaded source slot=%u enabled=%d host=%s label=%s urlHash=0x%08lx urlLen=%u alias=%d apiKey=%d color=%u",
          static_cast<unsigned>(index), out.enabled ? 1 : 0,
          normalized.host.c_str(),
          calendarSourceDiagnosticLabel(normalized.normalizedUrl).c_str(),
          static_cast<unsigned long>(calendarSourceDiagnosticHash(normalized.normalizedUrl)),
          static_cast<unsigned>(normalized.normalizedUrl.size()),
          out.alias.empty() ? 0 : 1, out.apiKey.empty() ? 0 : 1,
          static_cast<unsigned>(out.color));
    return true;
}

bool CalendarSecretStore::deleteSource(uint8_t index) {
    if (!isValidCalendarSourceIndex(index)) {
        return false;
    }
    const bool ok = removeAllSourceKeys(_backend, index);
    return _backend.commit() && ok;
}

std::string calendarSourceUrlKey(uint8_t index) {
    return indexedKey("url", index);
}

std::string calendarSourceApiKeyKey(uint8_t index) {
    return indexedKey("key", index);
}

std::string calendarSourceAliasKey(uint8_t index) {
    return indexedKey("alias", index);
}

std::string calendarSourceEnabledKey(uint8_t index) {
    return indexedKey("en", index);
}

std::string calendarSourceColorKey(uint8_t index) {
    return indexedKey("col", index);
}

std::string calendarSourceEtagKey(uint8_t index) {
    return indexedKey("etag", index);
}

std::string calendarSourceLastModifiedKey(uint8_t index) {
    return indexedKey("lm", index);
}

std::string calendarSourceErrorKey(uint8_t index) {
    return indexedKey("err", index);
}

std::string calendarSourceCacheMarkerKey(uint8_t index) {
    return indexedKey("cache", index);
}
