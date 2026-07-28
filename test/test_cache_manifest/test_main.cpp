#include <unity.h>

#include <cstring>

#include "app/cache/cache_store.h"
#include "app/cache/cache_store.cpp"

namespace {
std::vector<uint8_t> bytes(const char *text) {
    return std::vector<uint8_t>(text, text + strlen(text));
}

class FaultyCacheBackend final : public ICacheBackend {
public:
    bool exists(const std::string &path) const override {
        return _files.find(path) != _files.end();
    }

    bool readFile(const std::string &path, std::vector<uint8_t> &out) const override {
        const auto it = _files.find(path);
        if (it == _files.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    bool writeFile(const std::string &path, const std::vector<uint8_t> &data) override {
        _files[path] = data;
        return true;
    }

    bool removeFile(const std::string &path) override {
        _files.erase(path);
        return true;
    }

    bool renameFile(const std::string &from, const std::string &to) override {
        if (failTempToCurrentRename && from.find("temp.bin") != std::string::npos &&
            to.find("current.bin") != std::string::npos) {
            return false;
        }
        const auto it = _files.find(from);
        if (it == _files.end()) {
            return false;
        }
        _files[to] = it->second;
        _files.erase(it);
        return true;
    }

    bool failTempToCurrentRename = false;

private:
    std::map<std::string, std::vector<uint8_t>> _files;
};
}  // namespace

void test_cache_write_reads_current_payload() {
    MemoryCacheBackend backend;
    CacheStore store(backend, "/cache");

    TEST_ASSERT_TRUE(store.write("calendar", bytes("fresh"), 1000));
    const auto result = store.read("calendar");

    TEST_ASSERT_TRUE(result.ok);
    TEST_ASSERT_FALSE(result.fromPrevious);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(bytes("fresh").data(), result.payload.data(), result.payload.size());
}

void test_cache_falls_back_to_previous_when_current_crc_fails() {
    MemoryCacheBackend backend;
    CacheStore store(backend, "/cache");

    TEST_ASSERT_TRUE(store.write("calendar", bytes("old"), 1000));
    TEST_ASSERT_TRUE(store.write("calendar", bytes("new"), 1100));
    backend.writeFile("/cache/calendar/current.bin", bytes("bad"));

    const auto result = store.read("calendar");

    TEST_ASSERT_TRUE(result.ok);
    TEST_ASSERT_TRUE(result.fromPrevious);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(bytes("old").data(), result.payload.data(), result.payload.size());
}

void test_cache_returns_empty_when_both_versions_are_bad() {
    MemoryCacheBackend backend;
    CacheStore store(backend, "/cache");

    TEST_ASSERT_TRUE(store.write("calendar", bytes("old"), 1000));
    TEST_ASSERT_TRUE(store.write("calendar", bytes("new"), 1100));
    backend.writeFile("/cache/calendar/current.bin", bytes("bad-current"));
    backend.writeFile("/cache/calendar/previous.bin", bytes("bad-previous"));

    const auto result = store.read("calendar");

    TEST_ASSERT_FALSE(result.ok);
    TEST_ASSERT_TRUE(result.payload.empty());
}

void test_cache_preserves_previous_payload_when_current_replace_fails() {
    FaultyCacheBackend backend;
    CacheStore store(backend, "/cache");

    TEST_ASSERT_TRUE(store.write("calendar", bytes("old"), 1000));
    backend.failTempToCurrentRename = true;
    TEST_ASSERT_FALSE(store.write("calendar", bytes("new"), 1100));

    const auto result = store.read("calendar");

    TEST_ASSERT_TRUE(result.ok);
    TEST_ASSERT_TRUE(result.fromPrevious);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(bytes("old").data(), result.payload.data(), result.payload.size());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_cache_write_reads_current_payload);
    RUN_TEST(test_cache_falls_back_to_previous_when_current_crc_fails);
    RUN_TEST(test_cache_returns_empty_when_both_versions_are_bad);
    RUN_TEST(test_cache_preserves_previous_payload_when_current_replace_fails);
    return UNITY_END();
}
