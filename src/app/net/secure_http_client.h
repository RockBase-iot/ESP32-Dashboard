#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "app/model/dashboard_models.h"

struct SecureHttpRequest {
    std::string url;
    std::string etag;
    std::string lastModified;
    uint32_t maxBytes = 512U * 1024U;
    uint32_t timeoutMs = 20000;
    uint8_t redirectLimit = 3;
};

struct SecureHttpResponse {
    SourceState state = SourceState::Stale;
    int statusCode = 0;
    bool notModified = false;
    bool changed = false;
    std::vector<uint8_t> payload;
    std::string etag;
    std::string lastModified;
    std::string contentType;
    std::string contentEncoding;
    std::string transferEncoding;
    uint32_t bytesRead = 0;
    int32_t declaredSize = -1;
    int32_t streamResult = 0;
    bool complete = false;
};

class SecureHttpClient {
public:
    SecureHttpClient(const uint8_t *caBundle, size_t caBundleSize);

    SecureHttpResponse get(const SecureHttpRequest &request);

private:
    const uint8_t *_caBundle;
    size_t _caBundleSize;
};
