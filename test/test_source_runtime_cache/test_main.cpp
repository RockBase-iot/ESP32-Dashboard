#include <unity.h>

#include "app/cache/cache_store.h"
#include "app/cache/cache_store.cpp"
#include "app/net/secure_http_client.h"
#include "app/source/source_runtime_cache.h"
#include "app/source/source_runtime_cache.cpp"

static std::vector<uint8_t> bytes(const char *text) {
    return std::vector<uint8_t>(text, text + strlen(text));
}

void test_live_success_writes_current_cache() {
    MemoryCacheBackend backend;
    CacheStore cache(backend, "/cache");
    SecureHttpResponse response;
    response.state = SourceState::Ok;
    response.payload = bytes("live");

    const RuntimeSourcePayload result =
        resolveRuntimeSourcePayload(cache, "news/bbc", response, 100, "empty");

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok),
                            static_cast<uint8_t>(result.status.state));
    TEST_ASSERT_FALSE(result.fromCache);
    TEST_ASSERT_FALSE(result.empty);
    TEST_ASSERT_EQUAL_STRING("live", std::string(result.payload.begin(), result.payload.end()).c_str());
    TEST_ASSERT_TRUE(cache.read("news/bbc").ok);
}

void test_fetch_failure_uses_stale_cache_with_explained_message() {
    MemoryCacheBackend backend;
    CacheStore cache(backend, "/cache");
    TEST_ASSERT_TRUE(cache.write("finance/stooq", bytes("cached"), 80));

    SecureHttpResponse response;
    response.state = SourceState::Tls;

    const RuntimeSourcePayload result =
        resolveRuntimeSourcePayload(cache, "finance/stooq", response, 120, "empty");

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Stale),
                            static_cast<uint8_t>(result.status.state));
    TEST_ASSERT_TRUE(result.fromCache);
    TEST_ASSERT_FALSE(result.empty);
    TEST_ASSERT_EQUAL_INT64(80, result.status.lastSuccessUtc);
    TEST_ASSERT_NOT_EQUAL(-1, result.message.find("Using cached data"));
}

void test_fetch_failure_without_cache_returns_empty_state() {
    MemoryCacheBackend backend;
    CacheStore cache(backend, "/cache");
    SecureHttpResponse response;
    response.state = SourceState::NotFound;

    const RuntimeSourcePayload result =
        resolveRuntimeSourcePayload(cache, "economic/custom", response, 120,
                                    "Add an RSS or ICS feed");

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::NotFound),
                            static_cast<uint8_t>(result.status.state));
    TEST_ASSERT_FALSE(result.fromCache);
    TEST_ASSERT_TRUE(result.empty);
    TEST_ASSERT_EQUAL_STRING("Add an RSS or ICS feed", result.message.c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_live_success_writes_current_cache);
    RUN_TEST(test_fetch_failure_uses_stale_cache_with_explained_message);
    RUN_TEST(test_fetch_failure_without_cache_returns_empty_state);
    UNITY_END();
}

void loop() {}
