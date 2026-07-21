#include <unity.h>

#include "app/web/web_config_validation.h"
#include "app/calendar/calendar_source.cpp"
#include "app/web/web_config_validation.cpp"

void test_redacts_calendar_urls_api_keys_wifi_passwords_and_portfolio() {
    TEST_ASSERT_EQUAL_STRING("<configured>", redactKeyValueForLog("calendarUrl", "https://x/y?secret=1").c_str());
    TEST_ASSERT_EQUAL_STRING("<configured>", redactKeyValueForLog("apiKey", "secret").c_str());
    TEST_ASSERT_EQUAL_STRING("<redacted>", redactKeyValueForLog("WifiPSWD", "secret").c_str());
    TEST_ASSERT_EQUAL_STRING("<configured>", redactKeyValueForLog("portfolioPositions", "AAPL:1").c_str());
}

void test_validates_rotation_interval_and_https_urls() {
    TEST_ASSERT_TRUE(isAllowedRotationInterval(90));
    TEST_ASSERT_FALSE(isAllowedRotationInterval(45));
    TEST_ASSERT_TRUE(isSafeDashboardUrl("https://example.com/feed.ics"));
    TEST_ASSERT_FALSE(isSafeDashboardUrl("http://example.com/feed.ics"));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_redacts_calendar_urls_api_keys_wifi_passwords_and_portfolio);
    RUN_TEST(test_validates_rotation_interval_and_https_urls);
    UNITY_END();
}

void loop() {}
