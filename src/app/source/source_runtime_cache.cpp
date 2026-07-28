#include "source_runtime_cache.h"

#include <cstdio>

const char *sourceStateName(SourceState state) {
    switch (state) {
        case SourceState::Ok: return "Ok";
        case SourceState::Stale: return "Stale";
        case SourceState::Tls: return "TLS";
        case SourceState::Auth: return "Auth";
        case SourceState::RateLimit: return "Rate limit";
        case SourceState::NotFound: return "Not found";
        case SourceState::Parse: return "Parse";
        case SourceState::Limit: return "Limit";
    }
    return "Unknown";
}

RuntimeSourcePayload resolveRuntimeSourcePayload(CacheStore &cache,
                                                 const std::string &category,
                                                 const SecureHttpResponse &response,
                                                 int64_t nowUtc,
                                                 const std::string &emptyMessage) {
    RuntimeSourcePayload result;
    result.status.sourceId = category;
    result.status.lastAttemptUtc = nowUtc;

    if (response.state == SourceState::Ok && !response.payload.empty()) {
        cache.write(category, response.payload, nowUtc);
        result.status.state = SourceState::Ok;
        result.status.lastSuccessUtc = nowUtc;
        result.status.itemCount = static_cast<uint32_t>(response.payload.size());
        result.payload = response.payload;
        result.message = "Live data";
        return result;
    }

    const CacheReadResult cached = cache.read(category);
    if (cached.ok) {
        result.status.state = SourceState::Stale;
        result.status.lastSuccessUtc = cached.updatedUtc;
        result.status.itemCount = static_cast<uint32_t>(cached.payload.size());
        result.payload = cached.payload;
        result.fromCache = true;
        result.message = std::string("Using cached data; latest fetch failed: ") +
                         sourceStateName(response.state);
        return result;
    }

    result.status.state = response.state;
    result.empty = true;
    result.message = emptyMessage;
    return result;
}
