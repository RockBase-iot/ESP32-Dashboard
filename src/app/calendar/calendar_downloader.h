#pragma once

#include <stdint.h>

#include "app/cache/cache_store.h"
#include "app/model/dashboard_models.h"
#include "app/net/secure_http_client.h"
#include "app/security/secret_store.h"

struct CalendarDownloadResult {
    SourceStatus status;
    bool changed = false;
    uint32_t bytes = 0;
};

class CalendarDownloader {
public:
    CalendarDownloader(CalendarSecretStore &secrets, ISecretBackend &secretBackend,
                       CacheStore &cache, SecureHttpClient &http);

    CalendarDownloadResult downloadSource(uint8_t index, const FetchContext &context);

private:
    CalendarSecretStore &_secrets;
    ISecretBackend &_secretBackend;
    CacheStore &_cache;
    SecureHttpClient &_http;
};
