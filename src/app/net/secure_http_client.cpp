#include "secure_http_client.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "app/calendar/calendar_source.h"

namespace {
bool isRedirectStatus(int code) {
    return code == 301 || code == 302 || code == 303 || code == 307 || code == 308;
}

SourceState mapHttpStatus(int code) {
    if (code == 401 || code == 403) return SourceState::Auth;
    if (code == 404 || code == 410) return SourceState::NotFound;
    if (code == 429) return SourceState::RateLimit;
    if (code > 0) return SourceState::Stale;
    return SourceState::Tls;
}
}  // namespace

SecureHttpClient::SecureHttpClient(const uint8_t *caBundle, size_t caBundleSize)
    : _caBundle(caBundle), _caBundleSize(caBundleSize) {}

SecureHttpResponse SecureHttpClient::get(const SecureHttpRequest &request) {
    SecureHttpResponse response;
    auto urlPolicy = normalizeCalendarSourceUrl(request.url);
    if (!urlPolicy.ok) {
        response.state = SourceState::Tls;
        return response;
    }
    std::string currentUrl = urlPolicy.normalizedUrl;

    for (uint8_t hop = 0; hop <= request.redirectLimit; ++hop) {
        WiFiClientSecure client;
        if (!_caBundle || _caBundleSize == 0) {
            response.state = SourceState::Tls;
            return response;
        }
        client.setCACertBundle(_caBundle
#if ESP_ARDUINO_VERSION_MAJOR >= 3
                               , _caBundleSize  // 3.x requires explicit bundle size
#endif
        );
        client.setTimeout(request.timeoutMs / 1000U);

        HTTPClient http;
        http.setTimeout(request.timeoutMs);
        http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
        if (!http.begin(client, currentUrl.c_str())) {
            response.state = SourceState::Tls;
            return response;
        }
        if (!request.etag.empty()) {
            http.addHeader("If-None-Match", request.etag.c_str());
        }
        if (!request.lastModified.empty()) {
            http.addHeader("If-Modified-Since", request.lastModified.c_str());
        }
        const char *responseHeaders[] = {"ETag", "Last-Modified", "Location", "Retry-After"};
        http.collectHeaders(responseHeaders, 4);

        const int code = http.GET();
        response.statusCode = code;
        if (code == 304) {
            response.state = SourceState::Ok;
            response.notModified = true;
            http.end();
            return response;
        }
        if (isRedirectStatus(code)) {
            const std::string location = http.header("Location").c_str();
            http.end();
            const auto redirect = validateCalendarRedirectTarget(currentUrl, location, hop + 1);
            if (!redirect.ok) {
                response.state = SourceState::Tls;
                return response;
            }
            currentUrl = redirect.normalizedUrl;
            continue;
        }
        if (code != 200) {
            response.state = mapHttpStatus(code);
            http.end();
            return response;
        }

        const int declaredSize = http.getSize();
        if (declaredSize > 0 && static_cast<uint32_t>(declaredSize) > request.maxBytes) {
            response.state = SourceState::Limit;
            http.end();
            return response;
        }

        auto *stream = http.getStreamPtr(); // WiFiClient* (2.x) / NetworkClient* (3.x)
        uint8_t buffer[512];
        while (http.connected() && (declaredSize < 0 ||
                                    response.payload.size() < static_cast<size_t>(declaredSize))) {
            const size_t available = stream->available();
            if (available == 0) {
                delay(1);
                continue;
            }
            const size_t toRead = available < sizeof(buffer) ? available : sizeof(buffer);
            const int read = stream->readBytes(buffer, toRead);
            if (read <= 0) {
                break;
            }
            if (response.payload.size() + static_cast<size_t>(read) > request.maxBytes) {
                response.state = SourceState::Limit;
                http.end();
                return response;
            }
            response.payload.insert(response.payload.end(), buffer, buffer + read);
        }

        response.bytesRead = static_cast<uint32_t>(response.payload.size());
        response.etag = http.header("ETag").c_str();
        response.lastModified = http.header("Last-Modified").c_str();
        response.state = SourceState::Ok;
        response.changed = true;
        http.end();
        return response;
    }

    response.state = SourceState::Tls;
    return response;
}
