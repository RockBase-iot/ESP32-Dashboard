#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "app/model/dashboard_models.h"

struct FetchResult {
    std::string providerId;
    SourceState state = SourceState::Ok;
    bool changed = false;
    std::vector<uint8_t> payload;
    std::string etag;
    std::string lastModified;
    int64_t retryAfterUtc = 0;
    uint32_t itemCount = 0;
};

class IProvider {
public:
    virtual ~IProvider() = default;
    virtual const char *id() const = 0;
    virtual uint32_t ttlSeconds() const = 0;
    virtual FetchResult fetch(const FetchContext &context) = 0;
};
