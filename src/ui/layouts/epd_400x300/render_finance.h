#pragma once

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
void renderStockInfoPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot);
void renderPortfolioSummaryPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot);
void renderEconomicCalendarPage(IDrawSurface &surface, const FinancePageSnapshot &snapshot);
