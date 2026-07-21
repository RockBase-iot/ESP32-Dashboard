#include <unity.h>

#include "app/calendar/calendar_source.h"
#include "app/calendar/calendar_source.cpp"

void test_accepts_https_and_extracts_hostname() {
    const auto result = normalizeCalendarSourceUrl(
        "https://calendar.google.com/calendar/ical/user/basic.ics?token=abcd1234");

    TEST_ASSERT_TRUE(result.ok);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CalendarUrlPolicyStatus::Ok),
                            static_cast<uint8_t>(result.status));
    TEST_ASSERT_EQUAL_STRING("https://calendar.google.com/calendar/ical/user/basic.ics?token=abcd1234",
                             result.normalizedUrl.c_str());
    TEST_ASSERT_EQUAL_STRING("calendar.google.com", result.host.c_str());
}

void test_normalizes_webcal_to_https() {
    const auto result = normalizeCalendarSourceUrl("webcal://p42-caldav.icloud.com/published/2/example");

    TEST_ASSERT_TRUE(result.ok);
    TEST_ASSERT_EQUAL_STRING("https://p42-caldav.icloud.com/published/2/example",
                             result.normalizedUrl.c_str());
    TEST_ASSERT_EQUAL_STRING("p42-caldav.icloud.com", result.host.c_str());
}

void test_rejects_plain_http() {
    const auto result = normalizeCalendarSourceUrl("http://example.com/private.ics");

    TEST_ASSERT_FALSE(result.ok);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CalendarUrlPolicyStatus::RejectedScheme),
                            static_cast<uint8_t>(result.status));
}

void test_rejects_url_userinfo() {
    const auto result = normalizeCalendarSourceUrl("https://user:pass@example.com/private.ics");

    TEST_ASSERT_FALSE(result.ok);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CalendarUrlPolicyStatus::RejectedUserInfo),
                            static_cast<uint8_t>(result.status));
}

void test_rejects_urls_over_2048_bytes() {
    std::string longUrl = "https://example.com/";
    longUrl.append(2048, 'a');

    const auto result = normalizeCalendarSourceUrl(longUrl);

    TEST_ASSERT_FALSE(result.ok);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CalendarUrlPolicyStatus::RejectedTooLong),
                            static_cast<uint8_t>(result.status));
}

void test_rejects_insecure_redirect_target() {
    const auto result = validateCalendarRedirectTarget(
        "https://example.com/feed.ics", "http://example.com/new.ics", 1);

    TEST_ASSERT_FALSE(result.ok);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CalendarUrlPolicyStatus::RejectedScheme),
                            static_cast<uint8_t>(result.status));
}

void test_rejects_redirect_after_three_hops() {
    const auto result = validateCalendarRedirectTarget(
        "https://example.com/feed.ics", "https://example.com/fourth.ics", 4);

    TEST_ASSERT_FALSE(result.ok);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CalendarUrlPolicyStatus::RejectedRedirectLimit),
                            static_cast<uint8_t>(result.status));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_accepts_https_and_extracts_hostname);
    RUN_TEST(test_normalizes_webcal_to_https);
    RUN_TEST(test_rejects_plain_http);
    RUN_TEST(test_rejects_url_userinfo);
    RUN_TEST(test_rejects_urls_over_2048_bytes);
    RUN_TEST(test_rejects_insecure_redirect_target);
    RUN_TEST(test_rejects_redirect_after_three_hops);
    UNITY_END();
}

void loop() {}
