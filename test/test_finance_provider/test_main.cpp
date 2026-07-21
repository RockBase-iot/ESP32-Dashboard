#include <unity.h>

#include "app/finance/finance_models.h"
#include "app/finance/stooq_csv_provider.h"
#include "app/finance/keyed_quote_provider.h"
#include "app/finance/economic_feed_provider.h"
#include "app/finance/stooq_csv_provider.cpp"
#include "app/finance/keyed_quote_provider.cpp"
#include "app/finance/economic_feed_provider.cpp"

void test_stooq_csv_requires_source_and_asof_and_marks_delayed() {
    const auto parsed = parseStooqCsvQuotes(
        "Symbol,Date,Time,Open,High,Low,Close,Volume\n"
        "AAPL.US,2026-07-20,14:55:00,210.10,214.20,209.70,213.50,1200\n",
        "stooq", 1784568900LL);

    TEST_ASSERT_EQUAL_UINT32(1, parsed.quotes.size());
    TEST_ASSERT_EQUAL_STRING("AAPL.US", parsed.quotes[0].ticker.c_str());
    TEST_ASSERT_EQUAL_FLOAT(213.50f, parsed.quotes[0].price);
    TEST_ASSERT_TRUE(parsed.quotes[0].delayed);
    TEST_ASSERT_EQUAL_STRING("stooq", parsed.quotes[0].source.c_str());
    TEST_ASSERT_TRUE(parsed.quotes[0].asOfUtc > 0);

    const auto missingSource = parseStooqCsvQuotes(
        "Symbol,Date,Time,Open,High,Low,Close,Volume\n"
        "MSFT.US,2026-07-20,14:55:00,1,1,1,510.25,1\n",
        "", 1784568900LL);
    TEST_ASSERT_EQUAL_UINT32(0, missingSource.quotes.size());
}

void test_stooq_csv_rejects_missing_asof_and_non_numeric_price() {
    const auto parsed = parseStooqCsvQuotes(
        "Symbol,Date,Time,Open,High,Low,Close,Volume\n"
        "NOASOF,,14:55:00,210.10,214.20,209.70,213.50,1200\n"
        "NODATA,2026-07-20,14:55:00,210.10,214.20,209.70,N/D,1200\n",
        "stooq", 1784568900LL);

    TEST_ASSERT_EQUAL_UINT32(0, parsed.quotes.size());
    TEST_ASSERT_TRUE(parsed.partialFailure);
    TEST_ASSERT_EQUAL_UINT32(2, parsed.rejectedRows);
}

void test_stooq_csv_marks_partial_ticker_failure() {
    const auto parsed = parseStooqCsvQuotes(
        "Symbol,Date,Time,Open,High,Low,Close,Volume\n"
        "AAPL.US,2026-07-20,14:55:00,210.10,214.20,209.70,213.50,1200\n"
        "BROKEN,2026-07-20,14:55:00\n",
        "stooq", 1784568900LL);

    TEST_ASSERT_EQUAL_UINT32(1, parsed.quotes.size());
    TEST_ASSERT_TRUE(parsed.partialFailure);
    TEST_ASSERT_EQUAL_UINT32(1, parsed.rejectedRows);
}

void test_finance_quote_freshness_rejects_expired_quotes() {
    FinanceQuote quote{"AAPL.US", 213.50f, 3.40f, 1.62f, "USD", 1000, true, "stooq"};

    TEST_ASSERT_TRUE(isFinanceQuoteFresh(quote, 1300, 600));
    TEST_ASSERT_FALSE(isFinanceQuoteFresh(quote, 2000, 600));
}

void test_keyed_quote_provider_handles_429_retry_after() {
    const auto result = mapFinanceHttpStatus("alpha-vantage", 429, "120", "", 1784560000LL);

    TEST_ASSERT_EQUAL_STRING("alpha-vantage", result.providerId.c_str());
    TEST_ASSERT_EQUAL(SourceState::RateLimit, result.state);
    TEST_ASSERT_EQUAL_INT64(1784560120LL, result.retryAfterUtc);
    TEST_ASSERT_FALSE(result.changed);
}

void test_keyed_quote_provider_handles_http_date_retry_after() {
    const auto result = mapFinanceHttpStatus(
        "alpha-vantage", 429, "Mon, 20 Jul 2026 12:30:00 GMT", "", 1784560000LL);

    TEST_ASSERT_EQUAL(SourceState::RateLimit, result.state);
    TEST_ASSERT_EQUAL_INT64(1784550600LL, result.retryAfterUtc);
}

void test_keyed_quote_json_rejects_missing_asof_and_non_numeric_price() {
    const auto parsed = parseKeyedQuoteJson(
        "{\"quotes\":["
        "{\"ticker\":\"MISS\",\"price\":10.0,\"currency\":\"USD\"},"
        "{\"ticker\":\"BAD\",\"price\":\"N/D\",\"currency\":\"USD\",\"asOfUtc\":1784568900}"
        "]}",
        "custom", true, 1784568900LL);

    TEST_ASSERT_EQUAL_UINT32(0, parsed.quotes.size());
    TEST_ASSERT_TRUE(parsed.partialFailure);
    TEST_ASSERT_EQUAL_UINT32(2, parsed.rejectedRows);
}

void test_economic_feed_parses_rss_and_ics_events() {
    const auto rss = parseEconomicRssFeed(
        "<rss><channel><item><title>US CPI High</title>"
        "<pubDate>Mon, 20 Jul 2026 12:30:00 GMT</pubDate>"
        "<forecast>3.1%</forecast><previous>3.0%</previous>"
        "<source>custom-rss</source></item></channel></rss>",
        "custom-rss", 5);
    TEST_ASSERT_EQUAL_UINT32(1, rss.events.size());
    TEST_ASSERT_EQUAL_STRING("US", rss.events[0].region.c_str());
    TEST_ASSERT_EQUAL(FinanceImpact::High, rss.events[0].impact);
    TEST_ASSERT_EQUAL_STRING("custom-rss", rss.events[0].source.c_str());
    TEST_ASSERT_EQUAL_INT64(1784550600LL, rss.events[0].startsAtUtc);
    TEST_ASSERT_EQUAL_STRING("3.1%", rss.events[0].forecast.c_str());
    TEST_ASSERT_EQUAL_STRING("3.0%", rss.events[0].previous.c_str());

    const auto ics = parseEconomicIcsFeed(
        "BEGIN:VCALENDAR\r\nBEGIN:VEVENT\r\nSUMMARY:EU GDP Medium\r\n"
        "DTSTART:20260721T090000Z\r\n"
        "X-FORECAST:0.4%\r\nX-PREVIOUS:0.3%\r\n"
        "END:VEVENT\r\nEND:VCALENDAR\r\n",
        "custom-ics", 5);
    TEST_ASSERT_EQUAL_UINT32(1, ics.events.size());
    TEST_ASSERT_EQUAL_STRING("EU", ics.events[0].region.c_str());
    TEST_ASSERT_EQUAL(FinanceImpact::Medium, ics.events[0].impact);
    TEST_ASSERT_EQUAL_INT64(1784624400LL, ics.events[0].startsAtUtc);
    TEST_ASSERT_EQUAL_STRING("0.4%", ics.events[0].forecast.c_str());
    TEST_ASSERT_EQUAL_STRING("0.3%", ics.events[0].previous.c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_stooq_csv_requires_source_and_asof_and_marks_delayed);
    RUN_TEST(test_stooq_csv_rejects_missing_asof_and_non_numeric_price);
    RUN_TEST(test_stooq_csv_marks_partial_ticker_failure);
    RUN_TEST(test_finance_quote_freshness_rejects_expired_quotes);
    RUN_TEST(test_keyed_quote_provider_handles_429_retry_after);
    RUN_TEST(test_keyed_quote_provider_handles_http_date_retry_after);
    RUN_TEST(test_keyed_quote_json_rejects_missing_asof_and_non_numeric_price);
    RUN_TEST(test_economic_feed_parses_rss_and_ics_events);
    UNITY_END();
}

void loop() {}
