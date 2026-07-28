#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/cache/cache_store.h"
#include "app/model/dashboard_models.h"
#include "app/net/secure_http_client.h"

struct RuntimeSourcePayload {
    SourceStatus status;
    std::vector<uint8_t> payload;
    bool fromCache = false;
    bool empty = false;
    std::string message;
};

const char *sourceStateName(SourceState state);

RuntimeSourcePayload resolveRuntimeSourcePayload(CacheStore &cache,
                                                 const std::string &category,
                                                 const SecureHttpResponse &response,
                                                 int64_t nowUtc,
                                                 const std::string &emptyMessage);
