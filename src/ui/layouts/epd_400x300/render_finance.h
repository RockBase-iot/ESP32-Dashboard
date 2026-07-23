#pragma once

#include <stddef.h>

#include <string>
#include <vector>

#include "app/finance/finance_models.h"
#include "ui/canvas/draw_surface.h"

struct FinancePageSnapshot {
    std::vector<FinanceQuote> quotes;
    PortfolioSummary portfolio;
    std::vector<EconomicEvent> events;
    std::string updatedText;
};

FinancePageSnapshot sampleFinancePageSnapshot();
void renderStockInfoPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot,
                         size_t pageNumber = 7, size_t pageCount = 8);
void renderPortfolioSummaryPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot,
                                size_t pageNumber = 7, size_t pageCount = 8);
void renderEconomicCalendarPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot,
                                size_t pageNumber = 8, size_t pageCount = 8);
