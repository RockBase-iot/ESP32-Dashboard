#include <unity.h>

#include "app/calendar/calendar_source.h"
#include "app/calendar/calendar_source.cpp"
#include "app/security/secret_store.h"
#include "app/security/secret_store.cpp"

void test_uses_short_nvs_keys_for_twenty_calendar_sources() {
    TEST_ASSERT_EQUAL_STRING("url00", calendarSourceUrlKey(0).c_str());
    TEST_ASSERT_EQUAL_STRING("url19", calendarSourceUrlKey(19).c_str());
    TEST_ASSERT_EQUAL_STRING("key07", calendarSourceApiKeyKey(7).c_str());
    TEST_ASSERT_EQUAL_STRING("etag03", calendarSourceEtagKey(3).c_str());
}

void test_secret_metadata_masks_url_query_and_api_key() {
    MemorySecretBackend backend;
    CalendarSecretStore store(backend);
    CalendarSourceSecrets secrets;
    secrets.index = 2;
    secrets.url = "https://calendar.google.com/calendar/ical/private/basic.ics?token=SECRET1234";
    secrets.apiKey = "APIKEY-SECRET-9876";
    secrets.alias = "Work";
    secrets.enabled = true;
    secrets.color = 2;

    TEST_ASSERT_TRUE(store.saveSource(secrets));
    const auto meta = store.readMetadata(2);

    TEST_ASSERT_TRUE(meta.configured);
    TEST_ASSERT_TRUE(meta.enabled);
    TEST_ASSERT_EQUAL_UINT8(2, meta.color);
    TEST_ASSERT_EQUAL_STRING("Work", meta.alias.c_str());
    TEST_ASSERT_EQUAL_STRING("calendar.google.com", meta.host.c_str());
    TEST_ASSERT_NOT_EQUAL(-1, meta.maskedUrl.find("1234"));
    TEST_ASSERT_EQUAL(-1, meta.maskedUrl.find("SECRET"));
    TEST_ASSERT_EQUAL(-1, meta.maskedUrl.find("token="));
    TEST_ASSERT_NOT_EQUAL(-1, meta.maskedApiKey.find("9876"));
    TEST_ASSERT_EQUAL(-1, meta.maskedApiKey.find("APIKEY-SECRET"));
}

void test_read_secret_returns_empty_for_invalid_index() {
    MemorySecretBackend backend;
    CalendarSecretStore store(backend);

    TEST_ASSERT_FALSE(store.readMetadata(20).configured);
    TEST_ASSERT_FALSE(store.saveSource(CalendarSourceSecrets{20, "https://example.com/a.ics", "", "", true, 0}));
}

void test_delete_source_removes_url_key_state_and_cache_marker() {
    MemorySecretBackend backend;
    CalendarSecretStore store(backend);
    CalendarSourceSecrets secrets;
    secrets.index = 0;
    secrets.url = "https://outlook.office365.com/owa/calendar/private/calendar.ics?sig=gone9999";
    secrets.apiKey = "KEY9999";
    secrets.alias = "Office";
    secrets.enabled = true;
    secrets.color = 1;

    TEST_ASSERT_TRUE(store.saveSource(secrets));
    backend.setString(calendarSourceEtagKey(0), "etag-value");
    backend.setString(calendarSourceLastModifiedKey(0), "last-modified-value");
    backend.setString(calendarSourceErrorKey(0), "tls");
    backend.setString(calendarSourceCacheMarkerKey(0), "cached");

    TEST_ASSERT_TRUE(store.deleteSource(0));

    TEST_ASSERT_FALSE(store.readMetadata(0).configured);
    TEST_ASSERT_FALSE(backend.hasKey(calendarSourceUrlKey(0)));
    TEST_ASSERT_FALSE(backend.hasKey(calendarSourceApiKeyKey(0)));
    TEST_ASSERT_FALSE(backend.hasKey(calendarSourceEtagKey(0)));
    TEST_ASSERT_FALSE(backend.hasKey(calendarSourceLastModifiedKey(0)));
    TEST_ASSERT_FALSE(backend.hasKey(calendarSourceErrorKey(0)));
    TEST_ASSERT_FALSE(backend.hasKey(calendarSourceCacheMarkerKey(0)));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_uses_short_nvs_keys_for_twenty_calendar_sources);
    RUN_TEST(test_secret_metadata_masks_url_query_and_api_key);
    RUN_TEST(test_read_secret_returns_empty_for_invalid_index);
    RUN_TEST(test_delete_source_removes_url_key_state_and_cache_marker);
    UNITY_END();
}

void loop() {}
