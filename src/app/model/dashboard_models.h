#pragma once

#include <stdint.h>

#include <string>

#include "app/memory/capacity_profile.h"

enum class SourceState : uint8_t {
    Ok,
    Stale,
    Tls,
    Auth,
    RateLimit,
    NotFound,
    Parse,
    Limit,
};

struct SourceStatus {
    std::string sourceId;
    SourceState state = SourceState::Stale;
    int64_t lastAttemptUtc = 0;
    int64_t lastSuccessUtc = 0;
    int64_t retryAfterUtc = 0;
    uint32_t itemCount = 0;
};

struct FetchContext {
    int64_t nowUtc;
    uint32_t freeInternalHeap;
    const CapacityProfile &capacity;
};
