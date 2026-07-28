#include <unity.h>

#include <cstring>

#include "app/provider/provider_registry.h"
#include "app/provider/provider_registry.cpp"

namespace {
class StubProvider final : public IProvider {
public:
    StubProvider(const char *providerId, SourceState state, int64_t retryAfter = 0)
        : _id(providerId), _state(state), _retryAfter(retryAfter) {}

    const char *id() const override { return _id; }
    uint32_t ttlSeconds() const override { return 300; }

    FetchResult fetch(const FetchContext &) override {
        ++calls;
        FetchResult result;
        result.providerId = _id;
        result.state = _state;
        result.changed = _state == SourceState::Ok;
        result.retryAfterUtc = _retryAfter;
        result.payload.assign(_id, _id + strlen(_id));
        return result;
    }

    int calls = 0;

private:
    const char *_id;
    SourceState _state;
    int64_t _retryAfter;
};
}  // namespace

void test_provider_failure_does_not_stop_other_providers() {
    StubProvider calendar("calendar", SourceState::Tls);
    StubProvider weather("weather", SourceState::Ok);
    ProviderRegistry registry;
    TEST_ASSERT_TRUE(registry.add(calendar));
    TEST_ASSERT_TRUE(registry.add(weather));

    const CapacityProfile capacity{10, 6, 250, 512UL * 1024UL, 80UL * 1024UL, false};
    const FetchContext context{1000, 160 * 1024, capacity};
    const auto results = registry.fetch(context, {"calendar", "weather"});

    TEST_ASSERT_EQUAL_UINT(2, results.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Tls), static_cast<uint8_t>(results[0].state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SourceState::Ok), static_cast<uint8_t>(results[1].state));
    TEST_ASSERT_EQUAL_INT(1, calendar.calls);
    TEST_ASSERT_EQUAL_INT(1, weather.calls);
}

void test_duplicate_provider_id_is_rejected() {
    StubProvider first("calendar", SourceState::Ok);
    StubProvider duplicate("calendar", SourceState::Ok);
    ProviderRegistry registry;

    TEST_ASSERT_TRUE(registry.add(first));
    TEST_ASSERT_FALSE(registry.add(duplicate));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_provider_failure_does_not_stop_other_providers);
    RUN_TEST(test_duplicate_provider_id_is_rejected);
    return UNITY_END();
}
