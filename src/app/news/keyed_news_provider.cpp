#include "keyed_news_provider.h"

FetchResult KeyedNewsProvider::fetch(const FetchContext & /*context*/) {
    FetchResult result;
    result.providerId = _providerId;
    result.state = SourceState::Stale;
    result.changed = false;
    (void)_endpoint;
    return result;
}
