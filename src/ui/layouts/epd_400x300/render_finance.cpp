#include "render_finance.h"

#include <array>
#include <algorithm>
#include <cstdio>

#include "app/finance/portfolio_store.h"
#include "ui/components/calm_grid.h"

namespace {
void drawFinanceHeader(IDrawSurface &surface, const std::string &title, const std::string &rightText) {
    calm_grid::drawPageHeader(surface, title, rightText, "", 74);
}

void drawFinanceComplianceFooter(IDrawSurface &surface, const FinancePageSnapshot &snapshot) {
    surface.drawText(18, 270, "Delayed", kDashboardAccent, TextAlign::Left, 1);
    surface.drawText(82, 270, "Source", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(146, 270, "Updated", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(382, 270, calm_grid::fitText(surface, snapshot.updatedText, 92, 1),
                     kDashboardBlack, TextAlign::Right, 1);
    surface.drawText(18, 286, "Not investment advice", kDashboardBlack, TextAlign::Left, 1);
}

std::string moneyText(float value) {
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "$%.2f", static_cast<double>(value));
    return buffer;
}

std::string percentText(float value) {
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%+.2f%%", static_cast<double>(value));
    return buffer;
}

void drawQuoteRow(IDrawSurface &surface, const Rect &rect, const FinanceQuote &quote) {
    surface.drawLine(rect.x, static_cast<int16_t>(rect.y + rect.h - 1),
                     static_cast<int16_t>(rect.x + rect.w), static_cast<int16_t>(rect.y + rect.h - 1),
                     kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 18),
                     quote.ticker, kDashboardBlack, TextAlign::Left, 2);
    surface.drawText(static_cast<int16_t>(rect.x + 148), static_cast<int16_t>(rect.y + 18),
                     moneyText(quote.price), kDashboardBlack, TextAlign::Right, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 226), static_cast<int16_t>(rect.y + 18),
                     percentText(quote.changePercent),
                     quote.changePercent >= 0.0f ? kDashboardAccent : kDashboardBlack,
                     TextAlign::Right, 1);
    surface.drawText(static_cast<int16_t>(rect.x + rect.w - 4), static_cast<int16_t>(rect.y + 18),
                     quote.source, kDashboardBlack, TextAlign::Right, 1);
}

void drawEventRow(IDrawSurface &surface, const Rect &rect, const EconomicEvent &event) {
    surface.drawLine(rect.x, static_cast<int16_t>(rect.y + rect.h - 1),
                     static_cast<int16_t>(rect.x + rect.w), static_cast<int16_t>(rect.y + rect.h - 1),
                     kDashboardBlack);
    surface.drawText(static_cast<int16_t>(rect.x + 4), static_cast<int16_t>(rect.y + 18),
                     event.region, kDashboardAccent, TextAlign::Left, 2);
    surface.drawText(static_cast<int16_t>(rect.x + 54), static_cast<int16_t>(rect.y + 14),
                     calm_grid::fitText(surface, event.name, 190, 1),
                     kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(static_cast<int16_t>(rect.x + rect.w - 4), static_cast<int16_t>(rect.y + 14),
                     financeImpactLabel(event.impact), kDashboardBlack, TextAlign::Right, 1);
    surface.drawText(static_cast<int16_t>(rect.x + 54), static_cast<int16_t>(rect.y + 28),
                     calm_grid::fitText(surface, event.source, 190, 1),
                     kDashboardBlack, TextAlign::Left, 1);
}
}  // namespace

FinancePageSnapshot sampleFinancePageSnapshot() {
    PortfolioSnapshot portfolioInput;
    portfolioInput.positions = {
        {"AAPL.US", 2.0f, 100.0f, "USD"},
        {"MSFT.US", 1.0f, 220.0f, "USD"},
    };
    portfolioInput.quotes = {
        {"AAPL.US", 213.50f, 3.40f, 1.62f, "USD", 1784559300LL, true, "Stooq"},
        {"MSFT.US", 510.25f, -1.70f, -0.33f, "USD", 1784559300LL, true, "Stooq"},
        {"BTCUSD", 118240.0f, 240.0f, 0.20f, "USD", 1784559300LL, true, "Custom"},
    };

    FinancePageSnapshot snapshot;
    snapshot.quotes = portfolioInput.quotes;
    snapshot.portfolio = calculatePortfolioSummary(portfolioInput);
    snapshot.events = {
        {1784550600LL, "US", FinanceImpact::High, "US CPI", "3.1%", "3.0%", "custom-rss"},
        {1784624400LL, "EU", FinanceImpact::Medium, "EU GDP", "0.4%", "0.3%", "custom-ics"},
        {1784682000LL, "CN", FinanceImpact::Low, "CN PMI", "50.8", "50.6", "custom-rss"},
    };
    snapshot.updatedText = "08:30";
    return snapshot;
}

void renderStockInfoPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot) {
    drawFinanceHeader(surface, "STOCK INFO", "FINANCE   10/12");
    const std::array<Rect, 4> rows = {{{18, 58, 364, 42}, {18, 100, 364, 42},
                                       {18, 142, 364, 42}, {18, 184, 364, 42}}};
    const size_t count = std::min<size_t>(rows.size(), snapshot.quotes.size());
    for (size_t i = 0; i < count; ++i) {
        drawQuoteRow(surface, rows[i], snapshot.quotes[i]);
    }
    drawFinanceComplianceFooter(surface, snapshot);
}

void renderPortfolioSummaryPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot) {
    drawFinanceHeader(surface, "PORTFOLIO", "LOCAL ONLY   11/12");
    surface.drawRect(18, 62, 364, 76, kDashboardBlack);
    surface.drawText(28, 86, "Market value", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(28, 120, moneyText(snapshot.portfolio.marketValue), kDashboardAccent,
                     TextAlign::Left, 3);
    surface.drawText(226, 86, "Gain/Loss", kDashboardBlack, TextAlign::Left, 1);
    surface.drawText(226, 120, percentText(snapshot.portfolio.gainLossPercent), kDashboardBlack,
                     TextAlign::Left, 2);

    surface.drawText(18, 164, "TOP MOVERS", kDashboardBlack, TextAlign::Left, 1);
    const size_t count = std::min<size_t>(3, snapshot.portfolio.topMovers.size());
    for (size_t i = 0; i < count; ++i) {
        const PortfolioHolding &holding = snapshot.portfolio.topMovers[i];
        const int16_t y = static_cast<int16_t>(188 + 20 * static_cast<int16_t>(i));
        surface.drawText(28, y, holding.ticker, kDashboardBlack, TextAlign::Left, 1);
        surface.drawText(170, y, moneyText(holding.marketValue), kDashboardBlack, TextAlign::Right, 1);
        surface.drawText(274, y, percentText(holding.gainLossPercent),
                         holding.gainLossPercent >= 0.0f ? kDashboardAccent : kDashboardBlack,
                         TextAlign::Right, 1);
    }
    drawFinanceComplianceFooter(surface, snapshot);
}

void renderEconomicCalendarPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot) {
    drawFinanceHeader(surface, "ECONOMIC CALENDAR", "RSS/ICS   12/12");
    const std::array<Rect, 4> rows = {{{18, 58, 364, 42}, {18, 100, 364, 42},
                                       {18, 142, 364, 42}, {18, 184, 364, 42}}};
    const size_t count = std::min<size_t>(rows.size(), snapshot.events.size());
    for (size_t i = 0; i < count; ++i) {
        drawEventRow(surface, rows[i], snapshot.events[i]);
    }
    drawFinanceComplianceFooter(surface, snapshot);
}
