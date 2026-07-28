#include "calendar_downloader.h"

#include <cstdio>

namespace {
std::string sourceIdForIndex(uint8_t index) {
    char buffer[12];
    std::snprintf(buffer, sizeof(buffer), "cal%02u", static_cast<unsigned>(index));
    return std::string(buffer);
}

std::string cacheCategoryForIndex(uint8_t index) {
    return std::string("calendar/") + sourceIdForIndex(index);
}
}  // namespace

CalendarDownloader::CalendarDownloader(CalendarSecretStore &secrets, ISecretBackend &secretBackend,
                                       CacheStore &cache, SecureHttpClient &http)
    : _secrets(secrets), _secretBackend(secretBackend), _cache(cache), _http(http) {}

CalendarDownloadResult CalendarDownloader::downloadSource(uint8_t index,
                                                          const FetchContext &context) {
    CalendarDownloadResult result;
    result.status.sourceId = sourceIdForIndex(index);
    result.status.lastAttemptUtc = context.nowUtc;

    CalendarSourceSecrets source;
    if (!_secrets.loadSourceForDownload(index, source) || !source.enabled) {
        result.status.state = SourceState::Stale;
        return result;
    }

    SecureHttpRequest request;
    request.url = source.url;
    request.maxBytes = context.capacity.maxSourceBytes;
    _secretBackend.getString(calendarSourceEtagKey(index), request.etag);
    _secretBackend.getString(calendarSourceLastModifiedKey(index), request.lastModified);

    const SecureHttpResponse http = _http.get(request);
    result.status.state = http.state;
    result.bytes = http.bytesRead;
    if (http.notModified) {
        result.status.state = SourceState::Ok;
        result.status.lastSuccessUtc = context.nowUtc;
        return result;
    }
    if (http.state != SourceState::Ok) {
        _secretBackend.setString(calendarSourceErrorKey(index),
                                 std::to_string(static_cast<int>(http.state)));
        _secretBackend.commit();
        return result;
    }

    if (!_cache.write(cacheCategoryForIndex(index), http.payload, context.nowUtc)) {
        result.status.state = SourceState::Stale;
        return result;
    }
    _secretBackend.setString(calendarSourceEtagKey(index), http.etag);
    _secretBackend.setString(calendarSourceLastModifiedKey(index), http.lastModified);
    _secretBackend.setString(calendarSourceCacheMarkerKey(index), "1");
    _secretBackend.remove(calendarSourceErrorKey(index));
    _secretBackend.commit();

    result.changed = http.changed;
    result.status.lastSuccessUtc = context.nowUtc;
    return result;
}
