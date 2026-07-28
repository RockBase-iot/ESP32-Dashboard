#include "wikimedia_history_provider.h"

FetchResult WikimediaHistoryProvider::fetch(const FetchContext & /*context*/) {
    FetchResult result;
    result.providerId = id();
    result.state = SourceState::Stale;
    result.changed = false;
    return result;
}
