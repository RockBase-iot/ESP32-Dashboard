#include <unity.h>

#include "app/finance/finance_models.h"
#include "app/finance/portfolio_store.h"
#include "app/finance/portfolio_store.cpp"

void test_portfolio_computes_value_cost_and_gain_locally() {
    PortfolioSnapshot snapshot;
    snapshot.positions = {
        {"AAPL.US", 2.0f, 100.0f, "USD"},
        {"MSFT.US", 1.5f, 200.0f, "USD"},
    };
    snapshot.quotes = {
        {"AAPL.US", 120.0f, 1.0f, 0.84f, "USD", 1784568900LL, true, "stooq"},
        {"MSFT.US", 180.0f, -2.0f, -1.10f, "USD", 1784568900LL, true, "stooq"},
    };

    const auto summary = calculatePortfolioSummary(snapshot);

    TEST_ASSERT_EQUAL_FLOAT(510.0f, summary.marketValue);
    TEST_ASSERT_EQUAL_FLOAT(500.0f, summary.costBasis);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, summary.gainLoss);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, summary.gainLossPercent);
    TEST_ASSERT_EQUAL_STRING("AAPL.US", summary.topMovers[0].ticker.c_str());
}

void test_portfolio_rejects_negative_quantity_and_zero_cost_is_supported() {
    TEST_ASSERT_FALSE(isValidPortfolioPosition({"BAD", -1.0f, 10.0f, "USD"}));
    TEST_ASSERT_TRUE(isValidPortfolioPosition({"GIFT", 1.0f, 0.0f, "USD"}));

    PortfolioSnapshot snapshot;
    snapshot.positions = {{"GIFT", 1.0f, 0.0f, "USD"}};
    snapshot.quotes = {{"GIFT", 25.0f, 0.0f, 0.0f, "USD", 1784568900LL, true, "stooq"}};

    const auto summary = calculatePortfolioSummary(snapshot);
    TEST_ASSERT_EQUAL_FLOAT(25.0f, summary.marketValue);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, summary.costBasis);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, summary.gainLossPercent);
}

void test_portfolio_flags_missing_exchange_rate_or_quote_currency() {
    PortfolioSnapshot snapshot;
    snapshot.positions = {{"BMW.DE", 3.0f, 80.0f, "EUR"}};
    snapshot.quotes = {{"BMW.DE", 90.0f, 1.0f, 1.12f, "USD", 1784568900LL, true, "stooq"}};

    const auto summary = calculatePortfolioSummary(snapshot);

    TEST_ASSERT_EQUAL_UINT32(1, summary.missingQuotes);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, summary.marketValue);
}

void test_portfolio_export_excludes_positions_by_default() {
    PortfolioSnapshot snapshot;
    snapshot.positions = {{"AAPL.US", 2.0f, 100.0f, "USD"}};

    const std::string exported = exportPortfolioConfig(snapshot, false);

    TEST_ASSERT_TRUE(exported.find("AAPL") == std::string::npos);
    TEST_ASSERT_TRUE(exported.find("positions") == std::string::npos);
}

void test_portfolio_export_escapes_tickers_when_explicitly_included() {
    PortfolioSnapshot snapshot;
    snapshot.positions = {{"A\"B\\C", 2.0f, 100.0f, "USD"}};

    const std::string exported = exportPortfolioConfig(snapshot, true);

    TEST_ASSERT_TRUE(exported.find("A\\\"B\\\\C") != std::string::npos);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_portfolio_computes_value_cost_and_gain_locally);
    RUN_TEST(test_portfolio_rejects_negative_quantity_and_zero_cost_is_supported);
    RUN_TEST(test_portfolio_flags_missing_exchange_rate_or_quote_currency);
    RUN_TEST(test_portfolio_export_excludes_positions_by_default);
    RUN_TEST(test_portfolio_export_escapes_tickers_when_explicitly_included);
    UNITY_END();
}

void loop() {}
