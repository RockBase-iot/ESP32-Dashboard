#pragma once

#include <stddef.h>

#include <string>
#include <vector>

#include "app/finance/finance_models.h"
#include "ui/components/calm_grid.h"

struct FinancePageSnapshot {
    std::vector<FinanceQuote> quotes;
    PortfolioSummary portfolio;
    std::vector<EconomicEvent> events;
    std::string updatedText;
};

FinancePageSnapshot sampleFinancePageSnapshot();
void renderStockInfoPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot,
                         size_t pageNumber = 7, size_t pageCount = 8,
                         const std::string &ipText = "IP: --",
                         calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderPortfolioSummaryPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot,
                                size_t pageNumber = 7, size_t pageCount = 8,
                                const std::string &ipText = "IP: --",
                                calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
void renderEconomicCalendarPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot,
                                size_t pageNumber = 8, size_t pageCount = 8,
                                const std::string &ipText = "IP: --",
                                calm_grid::ChromeContext chrome = calm_grid::ChromeContext());
