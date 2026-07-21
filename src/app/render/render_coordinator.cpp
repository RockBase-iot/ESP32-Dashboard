#include "render_coordinator.h"

RenderDecision RenderCoordinator::decide(const RenderInputs &input) const {
    RenderDecision decision;
    decision.targetPage = input.requestedPage;

    if (input.epdBusy) {
        decision.shouldRender = false;
        decision.deferred = true;
        return decision;
    }

    const bool samePage = input.requestedPage == input.currentPage;
    const bool sameContent = input.contentHash != 0 && input.contentHash == input.lastContentHash;
    if (!input.forceRefresh && samePage && sameContent) {
        decision.shouldRender = false;
        return decision;
    }

    const bool hasRefreshTime = input.lastFullRefreshUtc > 0 && input.nowUtc > 0;
    const bool tooSoon = hasRefreshTime &&
                         input.nowUtc >= input.lastFullRefreshUtc &&
                         static_cast<uint64_t>(input.nowUtc - input.lastFullRefreshUtc) <
                             input.minFullRefreshIntervalSec;
    if (!input.forceRefresh && tooSoon) {
        decision.shouldRender = false;
        decision.deferred = true;
        return decision;
    }

    decision.shouldRender = true;
    return decision;
}
