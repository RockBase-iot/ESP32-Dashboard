#include "cache_store.h"

#include <LittleFS.h>

#include <cstdio>
#include <cstdlib>
#include <utility>

namespace {
constexpr uint32_t kCrcSeed = 0xFFFFFFFFUL;

std::vector<uint8_t> stringBytes(const std::string &text) {
    return std::vector<uint8_t>(text.begin(), text.end());
}

std::string bytesString(const std::vector<uint8_t> &bytes) {
    return std::string(bytes.begin(), bytes.end());
}

bool parseNumber(const std::string &manifest, const std::string &section,
                 const std::string &key, int64_t &out) {
    const std::string sectionNeedle = "\"" + section + "\"";
    const size_t sectionPos = manifest.find(sectionNeedle);
    if (sectionPos == std::string::npos) {
        return false;
    }
    const size_t sectionEnd = manifest.find('}', sectionPos);
    if (sectionEnd == std::string::npos) {
        return false;
    }
    const std::string keyNeedle = "\"" + key + "\"";
    const size_t keyPos = manifest.find(keyNeedle, sectionPos);
    if (keyPos == std::string::npos || keyPos > sectionEnd) {
        return false;
    }
    const size_t colon = manifest.find(':', keyPos);
    if (colon == std::string::npos || colon > sectionEnd) {
        return false;
    }
    char *end = nullptr;
    out = std::strtoll(manifest.c_str() + colon + 1, &end, 10);
    return end != manifest.c_str() + colon + 1;
}

CacheVersionMeta parseVersion(const std::string &manifest, const std::string &section) {
    CacheVersionMeta meta;
    int64_t value = 0;
    if (!parseNumber(manifest, section, "crc", value)) {
        return meta;
    }
    meta.crc = static_cast<uint32_t>(value);
    if (!parseNumber(manifest, section, "size", value)) {
        return CacheVersionMeta{};
    }
    meta.size = static_cast<uint32_t>(value);
    if (!parseNumber(manifest, section, "updatedUtc", value)) {
        return CacheVersionMeta{};
    }
    meta.updatedUtc = value;
    meta.present = true;
    return meta;
}

std::string versionJson(const CacheVersionMeta &meta) {
    if (!meta.present) {
        return "null";
    }
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer),
                  "{\"crc\":%lu,\"size\":%lu,\"updatedUtc\":%lld}",
                  static_cast<unsigned long>(meta.crc),
                  static_cast<unsigned long>(meta.size),
                  static_cast<long long>(meta.updatedUtc));
    return std::string(buffer);
}
}  // namespace

bool MemoryCacheBackend::exists(const std::string &path) const {
    return _files.find(path) != _files.end();
}

bool MemoryCacheBackend::readFile(const std::string &path, std::vector<uint8_t> &out) const {
    const auto it = _files.find(path);
    if (it == _files.end()) {
        return false;
    }
    out = it->second;
    return true;
}

bool MemoryCacheBackend::writeFile(const std::string &path, const std::vector<uint8_t> &data) {
    _files[path] = data;
    return true;
}

bool MemoryCacheBackend::removeFile(const std::string &path) {
    _files.erase(path);
    return true;
}

bool MemoryCacheBackend::renameFile(const std::string &from, const std::string &to) {
    const auto it = _files.find(from);
    if (it == _files.end()) {
        return false;
    }
    _files[to] = it->second;
    _files.erase(it);
    return true;
}

namespace {
bool ensureParentDirectories(const std::string &path) {
    size_t slash = 0;
    while (true) {
        slash = path.find('/', slash + 1);
        if (slash == std::string::npos) {
            return true;
        }
        if (slash == 0) {
            continue;
        }
        const std::string dir = path.substr(0, slash);
        if (!LittleFS.exists(dir.c_str()) && !LittleFS.mkdir(dir.c_str())) {
            return false;
        }
    }
}
}  // namespace

bool LittleFsCacheBackend::exists(const std::string &path) const {
    return LittleFS.exists(path.c_str());
}

bool LittleFsCacheBackend::readFile(const std::string &path, std::vector<uint8_t> &out) const {
    if (!LittleFS.exists(path.c_str())) {
        return false;
    }
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
        return false;
    }
    out.clear();
    out.reserve(static_cast<size_t>(file.size()));
    while (file.available()) {
        out.push_back(static_cast<uint8_t>(file.read()));
    }
    file.close();
    return true;
}

bool LittleFsCacheBackend::writeFile(const std::string &path, const std::vector<uint8_t> &data) {
    if (!ensureParentDirectories(path)) {
        return false;
    }
    File file = LittleFS.open(path.c_str(), "w");
    if (!file) {
        return false;
    }
    const size_t written = file.write(data.data(), data.size());
    file.close();
    return written == data.size();
}

bool LittleFsCacheBackend::removeFile(const std::string &path) {
    if (!LittleFS.exists(path.c_str())) {
        return true;
    }
    return LittleFS.remove(path.c_str());
}

bool LittleFsCacheBackend::renameFile(const std::string &from, const std::string &to) {
    if (!ensureParentDirectories(to)) {
        return false;
    }
    if (LittleFS.exists(to.c_str()) && !LittleFS.remove(to.c_str())) {
        return false;
    }
    return LittleFS.rename(from.c_str(), to.c_str());
}

CacheStore::CacheStore(ICacheBackend &backend, std::string rootPath)
    : _backend(backend), _rootPath(std::move(rootPath)) {}

bool CacheStore::write(const std::string &category, const std::vector<uint8_t> &payload,
                       int64_t updatedUtc) {
    const std::string tempPath = pathFor(category, "temp.bin");
    const std::string currentPath = pathFor(category, "current.bin");
    const std::string previousPath = pathFor(category, "previous.bin");

    CacheManifest manifest = readManifest(category);
    std::vector<uint8_t> oldCurrent;
    CacheVersionMeta oldCurrentMeta;
    if (_backend.readFile(currentPath, oldCurrent)) {
        oldCurrentMeta.present = true;
        oldCurrentMeta.crc = dashboardCrc32(oldCurrent);
        oldCurrentMeta.size = static_cast<uint32_t>(oldCurrent.size());
        oldCurrentMeta.updatedUtc = manifest.current.present ? manifest.current.updatedUtc : updatedUtc;
    }

    if (!_backend.writeFile(tempPath, payload)) {
        return false;
    }
    std::vector<uint8_t> verify;
    if (!_backend.readFile(tempPath, verify) || dashboardCrc32(verify) != dashboardCrc32(payload)) {
        _backend.removeFile(tempPath);
        return false;
    }

    if (oldCurrentMeta.present) {
        if (!_backend.writeFile(previousPath, oldCurrent)) {
            _backend.removeFile(tempPath);
            return false;
        }
        manifest.previous = oldCurrentMeta;
        if (!writeManifest(category, manifest)) {
            _backend.removeFile(tempPath);
            return false;
        }
    }

    if (!_backend.removeFile(currentPath)) {
        _backend.removeFile(tempPath);
        return false;
    }
    if (!_backend.renameFile(tempPath, currentPath)) {
        _backend.removeFile(tempPath);
        return false;
    }

    manifest.current.present = true;
    manifest.current.crc = dashboardCrc32(payload);
    manifest.current.size = static_cast<uint32_t>(payload.size());
    manifest.current.updatedUtc = updatedUtc;
    return writeManifest(category, manifest);
}

CacheReadResult CacheStore::read(const std::string &category) const {
    const CacheManifest manifest = readManifest(category);
    CacheReadResult result;
    if (readValidVersion(pathFor(category, "current.bin"), manifest.current, result.payload)) {
        result.ok = true;
        result.fromPrevious = false;
        result.updatedUtc = manifest.current.updatedUtc;
        return result;
    }
    if (readValidVersion(pathFor(category, "previous.bin"), manifest.previous, result.payload)) {
        result.ok = true;
        result.fromPrevious = true;
        result.updatedUtc = manifest.previous.updatedUtc;
        return result;
    }
    result.payload.clear();
    return result;
}

std::string CacheStore::pathFor(const std::string &category, const char *leaf) const {
    return _rootPath + "/" + category + "/" + leaf;
}

CacheManifest CacheStore::readManifest(const std::string &category) const {
    std::vector<uint8_t> bytes;
    if (!_backend.readFile(pathFor(category, "manifest.json"), bytes)) {
        return CacheManifest{};
    }
    const std::string manifestText = bytesString(bytes);
    CacheManifest manifest;
    manifest.current = parseVersion(manifestText, "current");
    manifest.previous = parseVersion(manifestText, "previous");
    return manifest;
}

bool CacheStore::writeManifest(const std::string &category, const CacheManifest &manifest) {
    const std::string body = std::string("{\"current\":") + versionJson(manifest.current) +
                             ",\"previous\":" + versionJson(manifest.previous) + "}";
    return _backend.writeFile(pathFor(category, "manifest.json"), stringBytes(body));
}

bool CacheStore::readValidVersion(const std::string &path, const CacheVersionMeta &meta,
                                  std::vector<uint8_t> &out) const {
    if (!meta.present) {
        return false;
    }
    std::vector<uint8_t> payload;
    if (!_backend.readFile(path, payload)) {
        return false;
    }
    if (payload.size() != meta.size) {
        return false;
    }
    if (dashboardCrc32(payload) != meta.crc) {
        return false;
    }
    out = payload;
    return true;
}

uint32_t dashboardCrc32(const std::vector<uint8_t> &data) {
    uint32_t crc = kCrcSeed;
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}
