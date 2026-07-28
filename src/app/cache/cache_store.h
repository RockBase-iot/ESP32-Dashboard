#pragma once

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

class ICacheBackend {
public:
    virtual ~ICacheBackend() = default;
    virtual bool exists(const std::string &path) const = 0;
    virtual bool readFile(const std::string &path, std::vector<uint8_t> &out) const = 0;
    virtual bool writeFile(const std::string &path, const std::vector<uint8_t> &data) = 0;
    virtual bool removeFile(const std::string &path) = 0;
    virtual bool renameFile(const std::string &from, const std::string &to) = 0;
};

class MemoryCacheBackend final : public ICacheBackend {
public:
    bool exists(const std::string &path) const override;
    bool readFile(const std::string &path, std::vector<uint8_t> &out) const override;
    bool writeFile(const std::string &path, const std::vector<uint8_t> &data) override;
    bool removeFile(const std::string &path) override;
    bool renameFile(const std::string &from, const std::string &to) override;

private:
    std::map<std::string, std::vector<uint8_t>> _files;
};

class LittleFsCacheBackend final : public ICacheBackend {
public:
    bool exists(const std::string &path) const override;
    bool readFile(const std::string &path, std::vector<uint8_t> &out) const override;
    bool writeFile(const std::string &path, const std::vector<uint8_t> &data) override;
    bool removeFile(const std::string &path) override;
    bool renameFile(const std::string &from, const std::string &to) override;
};

struct CacheReadResult {
    bool ok = false;
    bool fromPrevious = false;
    int64_t updatedUtc = 0;
    std::vector<uint8_t> payload;
};

struct CacheVersionMeta {
    bool present = false;
    uint32_t crc = 0;
    uint32_t size = 0;
    int64_t updatedUtc = 0;
};

struct CacheManifest {
    CacheVersionMeta current;
    CacheVersionMeta previous;
};

class CacheStore {
public:
    CacheStore(ICacheBackend &backend, std::string rootPath);

    bool write(const std::string &category, const std::vector<uint8_t> &payload, int64_t updatedUtc);
    CacheReadResult read(const std::string &category) const;

private:
    std::string pathFor(const std::string &category, const char *leaf) const;
    CacheManifest readManifest(const std::string &category) const;
    bool writeManifest(const std::string &category, const CacheManifest &manifest);
    bool readValidVersion(const std::string &path, const CacheVersionMeta &meta,
                          std::vector<uint8_t> &out) const;

    ICacheBackend &_backend;
    std::string _rootPath;
};

uint32_t dashboardCrc32(const std::vector<uint8_t> &data);
