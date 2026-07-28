#pragma once

#include <stdint.h>

#include <map>
#include <string>

#include "app/calendar/calendar_source.h"

class ISecretBackend {
public:
    virtual ~ISecretBackend() = default;
    virtual bool getString(const std::string &key, std::string &out) const = 0;
    virtual bool setString(const std::string &key, const std::string &value) = 0;
    virtual bool remove(const std::string &key) = 0;
    virtual bool commit() = 0;
};

class MemorySecretBackend final : public ISecretBackend {
public:
    bool getString(const std::string &key, std::string &out) const override;
    bool setString(const std::string &key, const std::string &value) override;
    bool remove(const std::string &key) override;
    bool commit() override;
    bool hasKey(const std::string &key) const;

private:
    std::map<std::string, std::string> _values;
};

class NvsSecretBackend final : public ISecretBackend {
public:
    explicit NvsSecretBackend(const char *nameSpace = "cal_src");
    ~NvsSecretBackend();

    bool getString(const std::string &key, std::string &out) const override;
    bool setString(const std::string &key, const std::string &value) override;
    bool remove(const std::string &key) override;
    bool commit() override;

private:
    void *_handle = nullptr;
};

class CalendarSecretStore {
public:
    explicit CalendarSecretStore(ISecretBackend &backend);

    bool saveSource(const CalendarSourceSecrets &source);
    CalendarSourceMetadata readMetadata(uint8_t index) const;
    bool loadSourceForDownload(uint8_t index, CalendarSourceSecrets &out) const;
    bool deleteSource(uint8_t index);

private:
    ISecretBackend &_backend;
};

std::string calendarSourceUrlKey(uint8_t index);
std::string calendarSourceApiKeyKey(uint8_t index);
std::string calendarSourceAliasKey(uint8_t index);
std::string calendarSourceEnabledKey(uint8_t index);
std::string calendarSourceColorKey(uint8_t index);
std::string calendarSourceEtagKey(uint8_t index);
std::string calendarSourceLastModifiedKey(uint8_t index);
std::string calendarSourceErrorKey(uint8_t index);
std::string calendarSourceCacheMarkerKey(uint8_t index);
