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

void test_normalizes_portal_window_seconds() {
    // Negative and zero both disable the power-on config window.
    TEST_ASSERT_EQUAL_UINT16(0, normalizePortalWindowSec(-5));
    TEST_ASSERT_EQUAL_UINT16(0, normalizePortalWindowSec(0));
    // Valid values pass through unchanged.
    TEST_ASSERT_EQUAL_UINT16(30, normalizePortalWindowSec(30));
    TEST_ASSERT_EQUAL_UINT16(600, normalizePortalWindowSec(600));
    // Anything above the 10-minute hard cap is clamped.
    TEST_ASSERT_EQUAL_UINT16(600, normalizePortalWindowSec(601));
    TEST_ASSERT_EQUAL_UINT16(600, normalizePortalWindowSec(3600));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_redacts_calendar_urls_api_keys_wifi_passwords_and_portfolio);
    RUN_TEST(test_validates_rotation_interval_and_https_urls);
    RUN_TEST(test_normalizes_portal_window_seconds);
    UNITY_END();
}

void loop() {}
