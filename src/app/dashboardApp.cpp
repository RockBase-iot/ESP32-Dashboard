#include "dashboardApp.h"
#include "bsp/IBoard.h"
#include "app/display/display_page_state.h"
#include "app/calendar/calendar_page_adapter.h"
#include "app/calendar/ics_parser.h"
#include "app/calendar/recurrence_engine.h"
#include "app/finance/economic_feed_provider.h"
#include "app/finance/portfolio_store.h"
#include "app/finance/stooq_csv_provider.h"
#include "app/input/button_controller.h"
#include "app/net/secure_http_client.h"
#include "app/news/rss_atom_provider.h"
#include "app/page/page_manager.h"
#include "app/power/wake_coordinator.h"
#include "app/render/render_coordinator.h"
#include "app/scheduler/sync_scheduler.h"
#include "app/security/secret_store.h"
#include "app/time/focus_clock_controller.h"
#include "app/time/focus_clock_model.h"
#include "ui/ui_layout.h"
#include "ui/components/calm_grid.h"
#if defined(UI_LAYOUT_EPD_400x300)
#include "ui/canvas/gfx_surface.h"
#include "ui/layouts/epd_400x300/render_agenda.h"
#include "ui/layouts/epd_400x300/render_calendar.h"
#include "ui/layouts/epd_400x300/render_finance.h"
#include "ui/layouts/epd_400x300/render_news.h"
#include "ui/layouts/epd_400x300/render_overview.h"
#include "ui/layouts/epd_400x300/render_time.h"
#include "ui/layouts/epd_400x300/render_weather.h"
#endif
#include "app/config/app_config.h"
#include "app/memory/capacity_profile.h"
#include "app/weather/weather.h"
#include "app/wifi/wifi_manager.h"
#include "app/web/web_server.h"
#include "app/locale/locale_mgr.h"
#include "utils/logger.h"
#include <WiFi.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <esp_attr.h>
#include <esp_sleep.h>
#include <esp_mac.h>
#include <time.h>
#include <utility>
#include <vector>

static const char *TAG = "DashboardApp";

extern const uint8_t _binary_src_assets_certs_mozilla_crt_bundle_bin_start[]
    asm("_binary_src_assets_certs_mozilla_crt_bundle_bin_start");
extern const uint8_t _binary_src_assets_certs_mozilla_crt_bundle_bin_end[]
    asm("_binary_src_assets_certs_mozilla_crt_bundle_bin_end");

// ── RTC-retained state (survives deep sleep) ───────────────────────────────
RTC_DATA_ATTR uint8_t DashboardApp::_failCount = 0;
RTC_DATA_ATTR bool    DashboardApp::_coldBoot  = true;
RTC_DATA_ATTR bool    DashboardApp::_apMode    = false;
RTC_DATA_ATTR bool    DashboardApp::_restorePersistedPage = false;
RTC_DATA_ATTR ButtonAction DashboardApp::_lastButtonAction = ButtonAction::None;
RTC_DATA_ATTR FocusClockRuntimeState DashboardApp::_focusClockState;

namespace {
#if defined(UI_LAYOUT_EPD_400x300)
FinancePageSnapshot gFinanceSnapshot;
NewsPageSnapshot gNewsSnapshot;
#endif

const char *buttonActionName(ButtonAction action) {
    switch (action) {
    case ButtonAction::NextPage: return "NextPage";
    case ButtonAction::PreviousPage: return "PreviousPage";
    case ButtonAction::SyncCurrent: return "SyncCurrent";
    case ButtonAction::OpenConfig: return "OpenConfig";
    case ButtonAction::RecoveryAp: return "RecoveryAp";
    case ButtonAction::None:
    default:
        return "None";
    }
}

uint32_t fnv1aAdd(uint32_t hash, uint32_t value) {
    for (uint8_t i = 0; i < 4; ++i) {
        hash ^= static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
        hash *= 16777619UL;
    }
    return hash;
}

uint32_t fnv1aAddString(uint32_t hash, const String &value) {
    for (size_t i = 0; i < value.length(); ++i) {
        hash ^= static_cast<uint8_t>(value[i]);
        hash *= 16777619UL;
    }
    return hash;
}

uint32_t scaledFloatHash(float value, float scale) {
    if (isnan(value)) {
        return 0xFFFFFFFFUL;
    }
    return static_cast<uint32_t>(static_cast<int32_t>(value * scale));
}

#if defined(UI_LAYOUT_EPD_400x300)
std::string asciiUpper(std::string value) {
    for (char &ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string trimAsciiCopy(const std::string &value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

struct WeatherLocationParts {
    std::string city;
    std::string region;
    std::string country;
};

WeatherLocationParts splitWeatherLocation(const String &value) {
    WeatherLocationParts parts;
    std::string raw = asciiUpper(value.c_str());
    raw = trimAsciiCopy(raw);
    if (raw.empty()) {
        return parts;
    }

    const size_t firstComma = raw.find(',');
    if (firstComma == std::string::npos) {
        parts.city = raw;
        return parts;
    }

    parts.city = trimAsciiCopy(raw.substr(0, firstComma));
    const size_t secondComma = raw.find(',', firstComma + 1);
    if (secondComma == std::string::npos) {
        parts.region = trimAsciiCopy(raw.substr(firstComma + 1));
        return parts;
    }

    parts.region = trimAsciiCopy(raw.substr(firstComma + 1, secondComma - firstComma - 1));
    parts.country = trimAsciiCopy(raw.substr(secondComma + 1));
    return parts;
}

std::string weatherConditionFromCode(int code, bool isDay) {
    if (code == 0) return isDay ? "Sunny" : "Clear";
    if (code == 1) return isDay ? "Mostly Sunny" : "Mostly Clear";
    if (code == 2) return "Partly Cloudy";
    if (code == 3) return "Cloudy";
    if (code == 45 || code == 48) return "Fog";
    if (code >= 51 && code <= 57) return "Drizzle";
    if (code >= 61 && code <= 67) return "Rain";
    if (code >= 71 && code <= 77) return "Snow";
    if (code >= 80 && code <= 82) return "Showers";
    if (code >= 95) return "Storm";
    return "Weather";
}

std::string formatIsoDateLabel(const String &isoDate, const char *format) {
    int year = 0, month = 0, day = 0;
    if (std::sscanf(isoDate.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
        return isoDate.c_str();
    }
    tm value = {};
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_isdst = -1;
    if (mktime(&value) == static_cast<time_t>(-1)) {
        return isoDate.c_str();
    }
    char buffer[32] = {};
    strftime(buffer, sizeof(buffer), format, &value);
    return asciiUpper(buffer);
}

std::string isoHourLabel(const String &isoDateTime) {
    const int tPos = isoDateTime.indexOf('T');
    if (tPos >= 0 && isoDateTime.length() >= static_cast<size_t>(tPos + 3)) {
        return isoDateTime.substring(tPos + 1, tPos + 3).c_str();
    }
    if (isoDateTime.length() >= 2) {
        return isoDateTime.substring(0, 2).c_str();
    }
    return isoDateTime.c_str();
}

int isoHourValue(const String &isoDateTime) {
    const std::string label = isoHourLabel(isoDateTime);
    if (label.size() < 2 || label[0] < '0' || label[0] > '9' ||
        label[1] < '0' || label[1] > '9') {
        return -1;
    }
    return (label[0] - '0') * 10 + (label[1] - '0');
}

bool isoDateMatches(const String &isoDateTime, const String &date) {
    return date.length() > 0 && isoDateTime.startsWith(date);
}

float toDisplayWindKph(float kph, const AppConfig &cfg) {
    if (cfg.unitsSpeed == "ms") return kph / 3.6f;
    if (cfg.unitsSpeed == "mph") return kph * 0.621371f;
    if (cfg.unitsSpeed == "kn") return kph * 0.539957f;
    return kph;
}

float toDisplayPressureHpa(float hpa, const AppConfig &cfg) {
    if (cfg.unitsPres == "inHg") return hpa * 0.02953f;
    if (cfg.unitsPres == "mmHg") return hpa * 0.750062f;
    return hpa;
}

float toDisplayDistanceKm(float km, const AppConfig &cfg) {
    if (cfg.unitsDist == "mi") return km * 0.621371f;
    return km;
}

WeatherPageSnapshot buildWeatherPageSnapshot(const WeatherClass &weather, const AppConfig &cfg) {
    if (!weather.weather().valid) {
        return sampleWeatherPageSnapshot();
    }

    const WeatherData &data = weather.weather();
    WeatherPageSnapshot snapshot;
    const WeatherLocationParts location = splitWeatherLocation(cfg.city);
    snapshot.city = location.city.empty() ? "WEATHER" : location.city;
    snapshot.region = location.region;
    snapshot.country = location.country;
    snapshot.updated = "Updated";
    snapshot.currentCondition = weatherConditionFromCode(data.current.weather_code, data.current.is_day);
    snapshot.tempUnit = cfg.unitsTemp == "F" ? "F" : "C";
    snapshot.currentTempC = static_cast<int>(std::round(data.current.temperature));
    snapshot.feelsLikeC = static_cast<int>(std::round(data.current.apparent_temperature));
    snapshot.humidityPct = static_cast<int>(std::round(data.current.humidity));
    snapshot.windKph = static_cast<int>(std::round(toDisplayWindKph(data.current.wind_speed, cfg)));
    snapshot.rainPct = 0;
    snapshot.pressureHpa = static_cast<int>(std::round(toDisplayPressureHpa(data.current.pressure, cfg)));
    snapshot.visibilityKm = static_cast<int>(std::round(toDisplayDistanceKm(data.current.visibility / 1000.0f, cfg)));
    snapshot.indoorTempC = snapshot.currentTempC;
    snapshot.indoorHumidityPct = snapshot.humidityPct;

    const size_t dailyCount = std::min<size_t>(7, data.daily.size());
    snapshot.weekly.reserve(dailyCount);
    for (size_t i = 0; i < dailyCount; ++i) {
        const WeatherDaily &day = data.daily[i];
        snapshot.weekly.push_back(WeatherDayCell{
            asciiUpper(formatIsoDateLabel(day.date, "%a")).substr(0, 3),
            day.weather_code,
            static_cast<int>(std::round(day.temp_max)),
            static_cast<int>(std::round(day.temp_min)),
            formatIsoDateLabel(day.date, "%b %d"),
            i == 0,
        });
    }

    const String todayDate = data.daily.empty() ? String() : data.daily.front().date;
    snapshot.hourly.reserve(8);
    for (size_t i = 0; i < data.hourly.size() && snapshot.hourly.size() < 8; ++i) {
        const WeatherHourly &hour = data.hourly[i];
        const int hourValue = isoHourValue(hour.time);
        if (!isoDateMatches(hour.time, todayDate) || hourValue < 0 || hourValue % 3 != 0) {
            continue;
        }
        snapshot.hourly.push_back(WeatherHourCell{
            isoHourLabel(hour.time),
            static_cast<int>(std::round(hour.temperature)),
            hour.precipitation_probability,
        });
    }
    for (size_t i = 0; snapshot.hourly.empty() && i < data.hourly.size() && snapshot.hourly.size() < 8;
         i += 3) {
        const WeatherHourly &hour = data.hourly[i];
        snapshot.hourly.push_back(WeatherHourCell{
            isoHourLabel(hour.time),
            static_cast<int>(std::round(hour.temperature)),
            hour.precipitation_probability,
        });
    }

    return snapshot;
}
#endif

uint32_t dashboardContentHash(PageId page, const WeatherClass &weather,
                              const AppConfig &cfg, const String &localIP,
                              int64_t nowUtc, uint32_t batteryMv) {
    uint32_t hash = 2166136261UL;
    hash = fnv1aAdd(hash, static_cast<uint32_t>(page));
    hash = fnv1aAdd(hash, static_cast<uint32_t>(nowUtc / 60LL));
    hash = fnv1aAdd(hash, batteryMv);
    hash = fnv1aAddString(hash, cfg.timeZoneId);
    hash = fnv1aAddString(hash, cfg.timeFormat);
    if (page == PageId::WeatherToday) {
        const WeatherData &data = weather.weather();
        const AirQualityData &aqi = weather.airQuality();
        hash = fnv1aAddString(hash, cfg.city);
        hash = fnv1aAddString(hash, cfg.unitsTemp);
        hash = fnv1aAddString(hash, cfg.unitsSpeed);
        hash = fnv1aAddString(hash, cfg.unitsPres);
        hash = fnv1aAddString(hash, cfg.unitsDist);
        hash = fnv1aAddString(hash, cfg.unitsPrecip);
        hash = fnv1aAdd(hash, data.valid ? 1 : 0);
        hash = fnv1aAdd(hash, scaledFloatHash(data.current.temperature, 10.0f));
        hash = fnv1aAdd(hash, scaledFloatHash(data.current.apparent_temperature, 10.0f));
        hash = fnv1aAdd(hash, scaledFloatHash(data.current.humidity, 1.0f));
        hash = fnv1aAdd(hash, static_cast<uint32_t>(data.current.weather_code));
        hash = fnv1aAdd(hash, static_cast<uint32_t>(data.daily.size()));
        hash = fnv1aAdd(hash, static_cast<uint32_t>(data.hourly.size()));
        hash = fnv1aAdd(hash, aqi.valid ? 1 : 0);
        hash = fnv1aAdd(hash, static_cast<uint32_t>(aqi.us_aqi));
        hash = fnv1aAddString(hash, localIP);
    } else if (page == PageId::FocusClock) {
        hash = fnv1aAddString(hash, cfg.focusLabel);
        hash = fnv1aAdd(hash, cfg.focusMinutes);
        hash = fnv1aAdd(hash, cfg.focusBreakMinutes);
        hash = fnv1aAdd(hash, cfg.focusSessionCount);
    }
    return hash == 0 ? 1 : hash;
}

PageId renderCoordinatorPageId(DisplayPageState page) {
    return page.isHomeWeather() ? static_cast<PageId>(0xFF) : page.managedPage();
}

uint32_t displayContentHash(DisplayPageState page, const WeatherClass &weather,
                            const AppConfig &cfg, const String &localIP,
                            int64_t nowUtc, uint32_t batteryMv) {
    const PageId hashPage = page.isHomeWeather() ? PageId::WeatherToday : page.managedPage();
    uint32_t hash = dashboardContentHash(hashPage, weather, cfg, localIP, nowUtc, batteryMv);
    if (page.isHomeWeather()) {
        hash = fnv1aAdd(hash, 0x484F4D45UL);
    }
    return hash == 0 ? 1 : hash;
}

bool isFocusDisplayPage(DisplayPageState page) {
    return !page.isHomeWeather() && page.managedPage() == PageId::FocusClock;
}

std::string chromeIpText(const String &localIP) {
    if (localIP.length() == 0 || localIP == "0.0.0.0") {
        return "IP: --";
    }
    return std::string("IP: ") + localIP.c_str();
}

bool isTwentyFourHourFormat(const String &timeFormat) {
    return timeFormat.indexOf("%I") < 0 && timeFormat.indexOf("%p") < 0;
}

FocusClockConfig focusClockConfigFromAppConfig(const AppConfig &cfg) {
    FocusClockConfig focus;
    focus.label = cfg.focusLabel.c_str();
    focus.focusMinutes = cfg.focusMinutes;
    focus.breakMinutes = cfg.focusBreakMinutes;
    focus.sessionCount = cfg.focusSessionCount;
    return normalizeFocusClockConfig(focus);
}

int batteryPercentFromMv(uint32_t batteryMv) {
    if (batteryMv <= 3300U) {
        return 0;
    }
    if (batteryMv >= 4200U) {
        return 100;
    }
    return static_cast<int>((batteryMv - 3300U) * 100U / 900U);
}

calm_grid::ChromeContext buildChromeContext(const AppConfig &cfg, int64_t nowUtc,
                                            const String &localIP, uint32_t batteryMv) {
    calm_grid::ChromeContext chrome;
    chrome.timeText = formatWorldClockChromeTimeLabel(nowUtc, cfg.timeZoneId.c_str(),
                                                     isTwentyFourHourFormat(cfg.timeFormat));
    chrome.ipText = chromeIpText(localIP);
    chrome.batteryText = batteryMv == 0
        ? "--"
        : std::to_string(batteryPercentFromMv(batteryMv)) + "%";
    return chrome;
}

std::string calendarSourceIdForIndex(uint8_t index) {
    char buffer[12];
    std::snprintf(buffer, sizeof(buffer), "cal%02u", static_cast<unsigned>(index));
    return std::string(buffer);
}

class CalendarVectorSink final : public IcsEventSink {
public:
    explicit CalendarVectorSink(std::string sourceLabel) : _sourceLabel(std::move(sourceLabel)) {}

    bool onEvent(const CalendarEvent &event) override {
        CalendarEvent copy = event;
        if (!_sourceLabel.empty()) {
            copy.sourceId = _sourceLabel;
        }
        events.push_back(std::move(copy));
        return true;
    }

    std::vector<CalendarEvent> events;

private:
    std::string _sourceLabel;
};

CalendarPageSnapshot syncCalendarPageSnapshot(const AppConfig &cfg, int64_t nowUtc,
                                              const CapacityProfile &capacity) {
    NvsSecretBackend backend;
    CalendarSecretStore store(backend);
    const auto certStart = _binary_src_assets_certs_mozilla_crt_bundle_bin_start;
    const auto certEnd = _binary_src_assets_certs_mozilla_crt_bundle_bin_end;
    SecureHttpClient http(certStart, static_cast<size_t>(certEnd - certStart));
    IcsParser parser;

    std::vector<CalendarEvent> rawEvents;
    uint8_t enabledSources = 0;
    for (uint8_t i = 0; i < capacity.calendarSlots && i < CALENDAR_SOURCE_MAX_COUNT; ++i) {
        CalendarSourceSecrets source;
        if (!store.loadSourceForDownload(i, source) || !source.enabled) {
            continue;
        }
        ++enabledSources;

        SecureHttpRequest request;
        request.url = source.url;
        request.maxBytes = capacity.maxSourceBytes;
        const SecureHttpResponse response = http.get(request);
        if (response.state != SourceState::Ok || response.payload.empty()) {
            backend.setString(calendarSourceErrorKey(i),
                              std::to_string(static_cast<int>(response.state)));
            backend.commit();
            log_w(TAG, "Calendar source %u fetch failed: state=%u status=%d bytes=%u",
                  static_cast<unsigned>(i), static_cast<unsigned>(response.state),
                  response.statusCode, static_cast<unsigned>(response.bytesRead));
            continue;
        }

        const std::string sourceId = calendarSourceIdForIndex(i);
        const std::string sourceLabel = source.alias.empty() ? sourceId : source.alias;
        StringIcsByteReader reader(std::string(response.payload.begin(), response.payload.end()));
        CalendarVectorSink sink(sourceLabel);
        const IcsParseResult parsed = parser.parse(sourceId, reader, sink);
        if (parsed.state != SourceState::Ok) {
            backend.setString(calendarSourceErrorKey(i),
                              std::to_string(static_cast<int>(SourceState::Parse)));
            backend.commit();
            log_w(TAG, "Calendar source %u parse failed: bytes=%u events=%u skipped=%u",
                  static_cast<unsigned>(i), static_cast<unsigned>(parsed.byteCount),
                  static_cast<unsigned>(parsed.eventCount),
                  static_cast<unsigned>(parsed.skippedCount));
            continue;
        }

        backend.setString(calendarSourceEtagKey(i), response.etag);
        backend.setString(calendarSourceLastModifiedKey(i), response.lastModified);
        backend.remove(calendarSourceErrorKey(i));
        backend.commit();
        rawEvents.insert(rawEvents.end(), sink.events.begin(), sink.events.end());
        log_i(TAG, "Calendar source %u OK: bytes=%u events=%u skipped=%u",
              static_cast<unsigned>(i), static_cast<unsigned>(response.bytesRead),
              static_cast<unsigned>(parsed.eventCount),
              static_cast<unsigned>(parsed.skippedCount));
    }

    if (enabledSources == 0) {
        return sampleCalendarPageSnapshot();
    }

    SourceState recurrenceState = SourceState::Ok;
    RecurrenceEngine recurrence;
    RecurrenceWindow window;
    window.startUtc = nowUtc - 7LL * 86400LL;
    window.endUtc = nowUtc + 35LL * 86400LL;
    window.seedUtc = nowUtc;
    window.capacity = capacity.maxExpandedEvents;
    std::vector<CalendarEvent> events = recurrence.expand(rawEvents, window,
                                                          cfg.timeZoneId.c_str(),
                                                          recurrenceState);
    if (recurrenceState != SourceState::Ok) {
        log_w(TAG, "Calendar recurrence expansion state=%u, using parsed events",
              static_cast<unsigned>(recurrenceState));
        events = rawEvents;
    }

    return calendarPageSnapshotFromEvents(events, nowUtc, cfg.timeZoneId.c_str(),
                                          isTwentyFourHourFormat(cfg.timeFormat));
}

#if defined(UI_LAYOUT_EPD_400x300)
std::vector<std::string> splitDashboardSourceList(const String &text) {
    std::vector<std::string> items;
    std::string token;
    auto flush = [&]() {
        token = trimAsciiCopy(token);
        if (!token.empty()) {
            items.push_back(token);
        }
        token.clear();
    };
    for (int i = 0; i < text.length(); ++i) {
        const char c = text.charAt(i);
        if (c == ',' || c == '\n' || c == '\r' || c == ';') {
            flush();
        } else {
            token.push_back(c);
        }
    }
    flush();
    return items;
}

bool isHttpsOrWebcalUrl(const std::string &value) {
    const std::string lower = asciiUpper(value);
    return lower.rfind("HTTPS://", 0) == 0 || lower.rfind("WEBCAL://", 0) == 0;
}

std::string hostLabelForUrl(const std::string &url, const char *fallback) {
    const auto normalized = normalizeCalendarSourceUrl(url);
    if (!normalized.ok || normalized.host.empty()) {
        return fallback;
    }
    std::string host = normalized.host;
    if (host.rfind("www.", 0) == 0) {
        host = host.substr(4);
    }
    const size_t dot = host.find('.');
    if (dot != std::string::npos && dot > 0) {
        host = host.substr(0, dot);
    }
    return asciiUpper(host);
}

std::string formatSyncTimeText(int64_t nowUtc) {
    time_t value = static_cast<time_t>(nowUtc);
    tm local = {};
#if defined(ESP_PLATFORM)
    localtime_r(&value, &local);
#else
    local = *std::localtime(&value);
#endif
    char buffer[12] = {};
    strftime(buffer, sizeof(buffer), "%H:%M", &local);
    return buffer;
}

std::string payloadToString(const SecureHttpResponse &response) {
    return std::string(response.payload.begin(), response.payload.end());
}

SecureHttpResponse fetchDashboardText(SecureHttpClient &http, const std::string &url,
                                      uint32_t maxBytes) {
    SecureHttpRequest request;
    request.url = url;
    request.maxBytes = maxBytes;
    request.redirectLimit = CALENDAR_REDIRECT_MAX_HOPS;
    return http.get(request);
}

FinancePageSnapshot syncFinancePageSnapshot(const AppConfig &cfg, int64_t nowUtc,
                                            const CapacityProfile &capacity) {
    FinancePageSnapshot snapshot;
    snapshot.updatedText = formatSyncTimeText(nowUtc);

    const auto certStart = _binary_src_assets_certs_mozilla_crt_bundle_bin_start;
    const auto certEnd = _binary_src_assets_certs_mozilla_crt_bundle_bin_end;
    SecureHttpClient http(certStart, static_cast<size_t>(certEnd - certStart));

    const std::array<std::string, 2> stockUrls = {
        buildStooqCsvUrl(cfg.stockSymbols.c_str()),
        buildStooqCsvUrlForHost("stooq.pl", cfg.stockSymbols.c_str()),
    };
    for (const std::string &stockUrl : stockUrls) {
        if (stockUrl.empty() || !snapshot.quotes.empty()) {
            continue;
        }
        const SecureHttpResponse response = fetchDashboardText(http, stockUrl, 64U * 1024U);
        if (response.state == SourceState::Ok && !response.payload.empty()) {
            const FinanceQuoteSet parsed = parseStooqCsvQuotes(payloadToString(response), "Stooq", nowUtc);
            snapshot.quotes = parsed.quotes;
            log_i(TAG, "Stooq quotes: items=%u rejected=%u partial=%d",
                  static_cast<unsigned>(snapshot.quotes.size()),
                  static_cast<unsigned>(parsed.rejectedRows), parsed.partialFailure ? 1 : 0);
        } else {
            log_w(TAG, "Stooq fetch failed: state=%u status=%d bytes=%u",
                  static_cast<unsigned>(response.state), response.statusCode,
                  static_cast<unsigned>(response.bytesRead));
        }
    }

    PortfolioSnapshot portfolioInput;
    portfolioInput.positions = parsePortfolioPositionsText(cfg.portfolioPositions.c_str());
    portfolioInput.quotes = snapshot.quotes;
    snapshot.portfolio = calculatePortfolioSummary(portfolioInput);

    for (const std::string &feed : splitDashboardSourceList(cfg.economicFeeds)) {
        if (!isHttpsOrWebcalUrl(feed)) {
            log_w(TAG, "Economic feed skipped because it is not a URL: %s", feed.c_str());
            continue;
        }
        const SecureHttpResponse response = fetchDashboardText(
            http, feed, std::min<uint32_t>(capacity.maxSourceBytes, 96U * 1024U));
        if (response.state != SourceState::Ok || response.payload.empty()) {
            log_w(TAG, "Economic feed failed: source=%s state=%u status=%d",
                  hostLabelForUrl(feed, "ECON").c_str(), static_cast<unsigned>(response.state),
                  response.statusCode);
            continue;
        }
        const std::string body = payloadToString(response);
        const std::string source = hostLabelForUrl(feed, "ECON");
        EconomicEventSet parsed = body.find("BEGIN:VCALENDAR") != std::string::npos
            ? parseEconomicIcsFeed(body, source, 8)
            : parseEconomicRssFeed(body, source, 8);
        snapshot.events.insert(snapshot.events.end(), parsed.events.begin(), parsed.events.end());
        log_i(TAG, "Economic feed OK: source=%s events=%u rejected=%u",
              source.c_str(), static_cast<unsigned>(parsed.events.size()),
              static_cast<unsigned>(parsed.rejectedItems));
        if (snapshot.events.size() >= 8) {
            break;
        }
    }
    std::sort(snapshot.events.begin(), snapshot.events.end(),
              [](const EconomicEvent &lhs, const EconomicEvent &rhs) {
                  return lhs.startsAtUtc < rhs.startsAtUtc;
              });
    if (snapshot.events.size() > 8) {
        snapshot.events.resize(8);
    }
    return snapshot;
}

NewsPageSnapshot syncNewsPageSnapshot(const AppConfig &cfg, const CapacityProfile &capacity) {
    NewsPageSnapshot snapshot;
    snapshot.title = "HEADLINES";
    snapshot.subtitle = "RSS";
    snapshot.history = sampleNewsPageSnapshot().history;

    const auto certStart = _binary_src_assets_certs_mozilla_crt_bundle_bin_start;
    const auto certEnd = _binary_src_assets_certs_mozilla_crt_bundle_bin_end;
    SecureHttpClient http(certStart, static_cast<size_t>(certEnd - certStart));

    for (const std::string &feed : splitDashboardSourceList(cfg.newsFeeds)) {
        if (!isHttpsOrWebcalUrl(feed)) {
            continue;
        }
        const SecureHttpResponse response = fetchDashboardText(
            http, feed, std::min<uint32_t>(capacity.maxSourceBytes, 96U * 1024U));
        const std::string fallbackSource = hostLabelForUrl(feed, "RSS");
        if (response.state != SourceState::Ok || response.payload.empty()) {
            log_w(TAG, "News feed failed: source=%s state=%u status=%d",
                  fallbackSource.c_str(), static_cast<unsigned>(response.state),
                  response.statusCode);
            continue;
        }
        const RssAtomFeed parsed = parseRssAtomTitles(payloadToString(response), 8, 150);
        for (const RssAtomItem &item : parsed.items) {
            NewsItemCell cell;
            cell.title = item.title;
            cell.source = item.source.empty() ? fallbackSource : item.source;
            cell.detail = item.link;
            cell.accent = snapshot.headlines.empty();
            snapshot.headlines.push_back(cell);
            if (snapshot.headlines.size() >= 6) {
                break;
            }
        }
        log_i(TAG, "News feed OK: source=%s items=%u total=%u",
              fallbackSource.c_str(), static_cast<unsigned>(parsed.items.size()),
              static_cast<unsigned>(snapshot.headlines.size()));
        if (snapshot.headlines.size() >= 6) {
            break;
        }
    }

    return snapshot;
}
#endif

RtcPageState snapshotDisplayRtcState(PageManager &pageManager, DisplayPageState page,
                                     int64_t lastFullRefreshUtc, uint32_t lastContentHash) {
    RtcPageState state = pageManager.snapshotRtcState(lastFullRefreshUtc, lastContentHash);
    state.currentPage = renderCoordinatorPageId(page);
    return state;
}

PageSettings pageSettingsFromConfig(const AppConfig &cfg) {
    PageSettings settings = defaultPageSettings();
    settings.configVersion = cfg.configVersion;
    settings.enabledMask = cfg.pageEnabledMask;
    settings.autoRotateMask = cfg.pageAutoRotateMask;
    settings.orderCount = cfg.pageOrderCount;
    for (size_t i = 0; i < settings.orderCount && i < settings.order.size(); ++i) {
        settings.order[i] = static_cast<PageId>(cfg.pageOrder[i]);
    }
    settings.templateId = static_cast<PageTemplateId>(cfg.pageTemplateId);
    settings.rotationIntervalMinutes = cfg.rotationIntervalMinutes;
    settings.timeZoneId = cfg.timeZoneId.c_str();
    return sanitizePageSettings(settings);
}

bool isPressedPin(uint8_t pin) {
    return pin != 0xFF && digitalRead(pin) == LOW;
}

void configureButtonInput(uint8_t pin) {
    if (pin != 0xFF) {
        pinMode(pin, INPUT_PULLUP);
    }
}

struct FocusHoldState {
    uint32_t pressedSinceMs = 0;
    bool handled = false;
};

ButtonAction pollFocusUserHold(DisplayPageState page, bool userPressed,
                               uint32_t nowMs, FocusHoldState &state) {
    if (!isFocusDisplayPage(page)) {
        state = FocusHoldState();
        return ButtonAction::None;
    }
    if (!userPressed) {
        state = FocusHoldState();
        return ButtonAction::None;
    }
    if (state.pressedSinceMs == 0) {
        state.pressedSinceMs = nowMs == 0 ? 1 : nowMs;
    }
    const uint32_t heldMs = nowMs - state.pressedSinceMs;
    const ButtonAction action = focusClockActionFromAwakeHold(page.managedPage(), true,
                                                              heldMs, state.handled);
    if (action != ButtonAction::None) {
        state.handled = true;
    }
    return action;
}

ButtonAction focusActionFromWakeButton(uint8_t userPin) {
    constexpr uint32_t kFocusStopHoldMs = 2000UL;
    if (userPin == 0xFF) {
        return ButtonAction::None;
    }
    if (!isPressedPin(userPin)) {
        return ButtonAction::None;
    }
    const uint32_t startMs = millis();
    while (isPressedPin(userPin) && millis() - startMs < kFocusStopHoldMs) {
        delay(20);
    }
    const uint32_t heldMs = millis() - startMs;
    return focusClockActionFromLightWake(LightWake::UserButton, heldMs);
}

}  // namespace

// ═════════════════════════════════════════════════════════════════════════════
// Phase 0 — Wakeup detection
// ═════════════════════════════════════════════════════════════════════════════
// Called before board.init() on EXT0 wakeup for accurate button timing.
// External pull-up: normal = HIGH, pressed = LOW.
// Held < 2 s → SHORT_PRESS (stay-awake); held ≥ 2 s → LONG_PRESS (AP mode).
void DashboardApp::_detectWakeup() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    bool isBootWake = (cause == ESP_SLEEP_WAKEUP_EXT0);

    _lastButtonAction = ButtonAction::None;
    _restorePersistedPage = (cause != ESP_SLEEP_WAKEUP_UNDEFINED);
    if (isBootWake) {
        uint8_t pin = getBoard().bootButtonPin();
        configureButtonInput(pin);
        unsigned long start = millis();
        while (digitalRead(pin) == LOW && millis() - start < 6500UL) {
            delay(10);
        }
        const uint32_t heldMs = static_cast<uint32_t>(millis() - start);
        if (digitalRead(pin) == LOW || heldMs >= 6000UL) {
            _lastButtonAction = ButtonAction::RecoveryAp;
        } else if (heldMs >= kLongPressMs) {
            _lastButtonAction = ButtonAction::OpenConfig;
        } else {
            _lastButtonAction = ButtonAction::None;
        }

        if (_lastButtonAction == ButtonAction::OpenConfig ||
            _lastButtonAction == ButtonAction::RecoveryAp) {
            _apMode = true;
        }
    }

    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        _coldBoot  = true;
        _failCount = 0;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 1 — Hardware init
// ═════════════════════════════════════════════════════════════════════════════
void DashboardApp::_initHardware(IBoard &board, bool coldBoot) {
    log_i(TAG, "Initializing board hardware");
    // Pass initialPowerOn=true only on cold boot; timer wake skips the EPD
    // power-on sequence to avoid a blank flash and speed up the refresh.
    board.epd().init(/*initialPowerOn=*/coldBoot);
    log_i(TAG, "EPD init done (%dx%d)", board.dispWidth(), board.dispHeight());

    if (board.getTempSensor()) {
        bool ok = board.getTempSensor()->begin();
        log_i(TAG, "Sensor %s init: %s", board.getTempSensor()->typeName(),
              ok ? "OK" : "FAILED");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2a — AP config mode (long press)
// ═════════════════════════════════════════════════════════════════════════════
// Starts a SoftAP + web portal, blocks for kApTimeoutMs, then restarts.
void DashboardApp::_enterApMode(IBoard &board) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char apSsid[32];
    snprintf(apSsid, sizeof(apSsid), "esp_dashboard_%02x%02x%02x",
             mac[3], mac[4], mac[5]);

    log_i(TAG, "Entering AP config mode: SSID=%s", apSsid);
    WifiManager apWifi;
    apWifi.startAP(apSsid);

    // Draw AP info after softAP starts so we can show the real IP.
    char apStatus[80];
    snprintf(apStatus, sizeof(apStatus), "AP: %s\nOpen 192.168.4.1 to configure",
             apSsid);
    (void)WiFi.softAPIP(); // IP is always 192.168.4.1 by default
    PageLoading apPage;
    apPage.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                  board.colorAccent(), board.hasAccentColor());
    apPage.setStatus(apStatus);
    board.epd().firstPage();
    do { apPage.draw(); } while (board.epd().nextPage());

    WebServer apWebServer;
    apWebServer.start();

    unsigned long start = millis();
    while (millis() - start < kApTimeoutMs) { delay(200); }

    log_i(TAG, "AP mode timeout — restarting");
    _apMode = false;
    ESP.restart();
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2b-i — Loading splash
// ═════════════════════════════════════════════════════════════════════════════
void DashboardApp::_showLoadingPage(IBoard &board, const char *status) {
    log_i(TAG, "Drawing loading page: %s", status);
    PageLoading loading;
    loading.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                   board.colorAccent(), board.hasAccentColor());
    loading.setStatus(status);
    board.epd().firstPage();
    do { loading.draw(); } while (board.epd().nextPage());
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2b-ii — WiFi connect + SNTP sync
// ═════════════════════════════════════════════════════════════════════════════
bool DashboardApp::_connectAndSync(IBoard &board, WifiManager &wifi, const AppConfig &cfg) {
    log_i(TAG, "Connecting WiFi: %s", cfg.wifiSsid.c_str());
    if (!wifi.connect(cfg.wifiSsid, cfg.wifiPassword)) {
        log_e(TAG, "WiFi connection failed");
        if (++_failCount >= kMaxFetchRetries) {
            _failCount = 0;
            _showErrorPage(board, "WiFi Error", "Could not connect to network.");
        }
        log_w(TAG, "Fail %d/%d — will retry on next wake", _failCount, kMaxFetchRetries);
        return false;
    }
    log_i(TAG, "WiFi connected, syncing time (UTC%+d)", cfg.utcOffset);
    wifi.syncTime(cfg.utcOffset);
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2b-iii — Weather + AQI fetch
// ═════════════════════════════════════════════════════════════════════════════
bool DashboardApp::_fetchData(IBoard &board, WeatherClass &weather,
                              const AppConfig &cfg, String &outLocalIP,
                              bool fetchWeather) {
    outLocalIP = WiFi.localIP().toString();
    log_i(TAG, "Local IP: %s", outLocalIP.c_str());
    if (!fetchWeather) {
        log_i(TAG, "Weather fetch skipped for current page");
        _failCount = 0;
        return true;
    }

    double lat = cfg.lat.toDouble();
    double lon = cfg.lon.toDouble();
    log_i(TAG, "Fetching weather: lat=%.4f lon=%.4f", lat, lon);

    bool weatherOk = weather.fetchWeather(lat, lon);
    log_i(TAG, "Weather fetch: %s", weatherOk ? "OK" : "FAILED");

    if (weatherOk) {
        log_i(TAG, "Fetching AQI");
        bool aqiOk = weather.fetchAirQuality(lat, lon);
        log_i(TAG, "AQI fetch: %s", aqiOk ? "OK" : "FAILED");
    }

    if (!weatherOk) {
        if (++_failCount >= kMaxFetchRetries) {
            _failCount = 0;
            _showErrorPage(board, "Weather Error", "Failed to fetch weather data.");
        }
        log_w(TAG, "Fail %d/%d — will retry on next wake", _failCount, kMaxFetchRetries);
        return false;
    }

    _failCount = 0;
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2b-iv — Weather page render
// ═════════════════════════════════════════════════════════════════════════════
void DashboardApp::_renderWeather(IBoard &board, WeatherClass &weather,
                                  const AppConfig &cfg, const String &localIP) {
    log_i(TAG, "Rendering weather page (locale=%s)", cfg.language.c_str());
    const LocaleData &loc = getLocale(cfg.language.c_str());
    PageWeather page;
    page.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                board.colorAccent(), board.hasAccentColor());
    page.setWeatherData(weather.weather(), weather.airQuality(), loc, cfg);
    page.setLocalIP(localIP);

    if (board.getTempSensor()) {
        float indoorTemp = NAN, indoorHumi = NAN, indoorPres = NAN;
        if (board.getTempSensor()->read(indoorTemp, indoorHumi, indoorPres)) {
            log_i(TAG, "Indoor: %.1f°C  %.0f%%", indoorTemp, indoorHumi);
            page.setIndoorData(indoorTemp, indoorHumi);
        } else {
            log_e(TAG, "Indoor sensor read failed");
        }
    }

    board.epd().firstPage();
    do { page.draw(); } while (board.epd().nextPage());
    log_i(TAG, "Render complete");
}

void DashboardApp::_renderDashboardPage(IBoard &board, PageManager &pageManager, PageId page,
                                        int64_t nowUtc, WeatherClass &weather,
                                        const CalendarPageSnapshot &calendarSnapshot,
                                        const AppConfig &cfg, const String &localIP,
                                        const calm_grid::ChromeContext &chrome) {
    const PageDescriptor *descriptor = findPage(page);
    log_i(TAG, "Rendering dashboard page: %s (%u)",
          descriptor ? descriptor->name : "Unknown", static_cast<unsigned>(page));
#if defined(UI_LAYOUT_EPD_400x300)
    GfxSurface surface(board.gfx());
    const size_t pageNumber = pageManager.pageNumber(page);
    const size_t pageCount = pageManager.pageCount();
    board.epd().firstPage();
    do {
        switch (page) {
            case PageId::WeatherToday:
                renderWeatherTodayPage(surface, buildWeatherPageSnapshot(weather, cfg),
                                       pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::Overview:
                renderOverviewPage(surface, calendarSnapshot, pageNumber, pageCount,
                                   chrome.ipText, chrome);
                break;
            case PageId::TodayAgenda:
                renderTodayAgendaPage(surface, calendarSnapshot, pageNumber, pageCount,
                                      chrome.ipText, chrome);
                break;
            case PageId::WeeklyTimeline:
                renderWeeklyTimelinePage(surface, calendarSnapshot, pageNumber,
                                         pageCount, chrome.ipText, chrome);
                break;
            case PageId::MonthlyOverview:
                renderMonthlyOverviewPage(surface, calendarSnapshot, pageNumber,
                                          pageCount, chrome.ipText, chrome);
                break;
            case PageId::LocalNotes:
                renderLocalNotesPage(surface, calendarSnapshot, pageNumber, pageCount,
                                     chrome.ipText, chrome);
                break;
            case PageId::WeeklyWeather:
                renderWeeklyWeatherPage(surface, buildWeatherPageSnapshot(weather, cfg),
                                        pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::IndoorClimate:
                renderIndoorClimatePage(surface, buildWeatherPageSnapshot(weather, cfg),
                                        pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::WorldClock:
                renderWorldClockPage(surface,
                                      worldClockPageSnapshotAt(nowUtc, cfg.timeZoneId.c_str(),
                                                               pageNumber, pageCount,
                                                               worldClockConfigFromZonesText(
                                                                   cfg.worldClockZones.c_str(),
                                                                   cfg.focusLabel.c_str(),
                                                                   isTwentyFourHourFormat(cfg.timeFormat))),
                                      pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::FocusClock:
                renderFocusClockPage(surface,
                                     focusClockPageSnapshotAt(nowUtc, cfg.timeZoneId.c_str(),
                                                              pageNumber, pageCount,
                                                              focusClockConfigFromAppConfig(cfg),
                                                              _focusClockState,
                                                              isTwentyFourHourFormat(cfg.timeFormat)),
                                     pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::StockInfo:
                renderStockInfoPage(surface, gFinanceSnapshot, pageNumber, pageCount,
                                    chrome.ipText, chrome);
                break;
            case PageId::PortfolioSummary:
                renderPortfolioSummaryPage(surface, gFinanceSnapshot,
                                           pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::EconomicCalendar:
                renderEconomicCalendarPage(surface, gFinanceSnapshot,
                                           pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::Headlines:
                renderHeadlinesPage(surface, gNewsSnapshot, pageNumber, pageCount,
                                    chrome.ipText, chrome);
                break;
            case PageId::TodayInHistory:
                renderTodayInHistoryPage(surface, gNewsSnapshot,
                                         pageNumber, pageCount, chrome.ipText, chrome);
                break;
            case PageId::ImportantMilestones:
                renderImportantMilestonesPage(surface, calendarSnapshot,
                                              pageNumber, pageCount, chrome.ipText, chrome);
                break;
            default:
                break;
        }
    } while (board.epd().nextPage());
    log_i(TAG, "Render complete");
#else
    _renderWeather(board, weather, cfg, localIP);
#endif
}

void DashboardApp::_renderDisplayPage(IBoard &board, PageManager &pageManager,
                                      DisplayPageState page, int64_t nowUtc,
                                      WeatherClass &weather,
                                      const CalendarPageSnapshot &calendarSnapshot,
                                      const AppConfig &cfg, const String &localIP,
                                      const calm_grid::ChromeContext &chrome) {
    if (page.isHomeWeather()) {
        _renderWeather(board, weather, cfg, localIP);
        return;
    }
    _renderDashboardPage(board, pageManager, page.managedPage(),
                         nowUtc, weather, calendarSnapshot, cfg, localIP, chrome);
}

// ═════════════════════════════════════════════════════════════════════════════
// Utility — error page
// ═════════════════════════════════════════════════════════════════════════════
DisplayPageState DashboardApp::_runFocusClockSession(IBoard &board, PageManager &pageManager,
                                                     DisplayPageState currentPage,
                                                     WeatherClass &weather,
                                                     const CalendarPageSnapshot &calendarSnapshot,
                                                     const AppConfig &cfg,
                                                     const String &localIP) {
    if (!isFocusDisplayPage(currentPage)) {
        return currentPage;
    }

    const FocusClockConfig focusConfig = focusClockConfigFromAppConfig(cfg);
    if (!focusClockSessionIsActive(focusConfig, _focusClockState,
                                   static_cast<int64_t>(time(nullptr)))) {
        return currentPage;
    }

    configureButtonInput(board.bootButtonPin());
    configureButtonInput(board.apButtonPin());
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    log_i(TAG, "Focus Clock active: WiFi off, entering minute light-sleep refresh loop");

    while (isFocusDisplayPage(currentPage)) {
        const int64_t nowUtc = static_cast<int64_t>(time(nullptr));
        if (!focusClockSessionIsActive(focusConfig, _focusClockState, nowUtc)) {
            const uint32_t batteryMv = board.readBatteryMv();
            const auto chrome = buildChromeContext(cfg, nowUtc, localIP, batteryMv);
            _renderDisplayPage(board, pageManager, currentPage, nowUtc, weather,
                               calendarSnapshot, cfg, localIP, chrome);
            const uint32_t contentHash = displayContentHash(currentPage, weather, cfg, localIP,
                                                            nowUtc, batteryMv);
            gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, currentPage,
                                                             nowUtc, contentHash);
            log_i(TAG, "Focus Clock complete");
            break;
        }

        const uint32_t refreshMs = focusClockNextRefreshMs(focusConfig, _focusClockState, nowUtc);
        if (refreshMs == 0) {
            continue;
        }

        const LightWake wake = board.lightSleepMs(refreshMs);
        const int64_t wakeUtc = static_cast<int64_t>(time(nullptr));
        log_i(TAG, "Focus Clock light wake: wake=%d refreshMs=%lu",
              static_cast<int>(wake), static_cast<unsigned long>(refreshMs));

        if (wake == LightWake::UserButton) {
            const ButtonAction action = focusActionFromWakeButton(board.apButtonPin());
            if (action == ButtonAction::SyncCurrent &&
                applyFocusClockButtonAction(currentPage.managedPage(), action, focusConfig,
                                            _focusClockState, wakeUtc)) {
                _lastButtonAction = action;
                const uint32_t batteryMv = board.readBatteryMv();
                const auto chrome = buildChromeContext(cfg, wakeUtc, localIP, batteryMv);
                _renderDisplayPage(board, pageManager, currentPage, wakeUtc, weather,
                                   calendarSnapshot, cfg, localIP, chrome);
                const uint32_t contentHash = displayContentHash(currentPage, weather, cfg, localIP,
                                                                wakeUtc, batteryMv);
                gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, currentPage,
                                                                 wakeUtc, contentHash);
                log_i(TAG, "Focus Clock stopped by USER long press");
                break;
            }
        }

        const uint32_t batteryMv = board.readBatteryMv();
        const auto chrome = buildChromeContext(cfg, wakeUtc, localIP, batteryMv);
        _renderDisplayPage(board, pageManager, currentPage, wakeUtc, weather,
                           calendarSnapshot, cfg, localIP, chrome);
        const uint32_t contentHash = displayContentHash(currentPage, weather, cfg, localIP,
                                                        wakeUtc, batteryMv);
        gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, currentPage,
                                                         wakeUtc, contentHash);
    }

    return currentPage;
}

void DashboardApp::_showErrorPage(IBoard &board, const char *title, const char *msg) {
    PageError errorPage;
    errorPage.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                     board.colorAccent(), board.hasAccentColor());
    errorPage.setMessage(title, msg);
    board.epd().firstPage();
    do { errorPage.draw(); } while (board.epd().nextPage());
}

DisplayPageState DashboardApp::_runInteractiveWindow(IBoard &board, PageManager &pageManager,
                                                     DisplayPageState currentPage,
                                                     WeatherClass &weather,
                                                     const CalendarPageSnapshot &calendarSnapshot,
                                                     const AppConfig &cfg,
                                                     const String &localIP) {
    const uint8_t bootPin = board.bootButtonPin();
    const uint8_t userPin = board.apButtonPin();
    configureButtonInput(bootPin);
    configureButtonInput(userPin);

    const uint32_t releaseStart = millis();
    while ((isPressedPin(bootPin) || isPressedPin(userPin)) &&
           millis() - releaseStart < 3000UL) {
        delay(20);
    }

    uint32_t lastActivityMs = millis();
    ButtonController buttons;
    FocusHoldState focusHold;
    WakeCoordinator wakeCoordinator;
    while (true) {
        const uint32_t nowMs = millis();
        WakeInputs wakeInput;
        wakeInput.currentState = PowerState::Interactive;
        wakeInput.signal = WakeSignal::Inactivity;
        wakeInput.secondsSinceActivity = (nowMs - lastActivityMs) / 1000UL;
        wakeInput.interactiveIdleSeconds = kInteractiveIdleSec;
        if (wakeCoordinator.decide(wakeInput).nextState == PowerState::DeepSleep) {
            break;
        }

        ButtonAction action = ButtonAction::None;
        const bool userPressed = isPressedPin(userPin);
        action = pollFocusUserHold(currentPage, userPressed, nowMs, focusHold);
        if (action == ButtonAction::None && bootPin != 0xFF) {
            action = buttons.update(ButtonId::Boot, isPressedPin(bootPin), nowMs);
        }
        if (action == ButtonAction::None) {
            if (userPin != 0xFF && !isFocusDisplayPage(currentPage)) {
                action = buttons.update(ButtonId::User, userPressed, nowMs);
            }
        }
        if (action == ButtonAction::None) {
            delay(20);
            continue;
        }

        _lastButtonAction = action;
        log_i(TAG, "Interactive button action: %s", buttonActionName(action));

        if (action == ButtonAction::OpenConfig || action == ButtonAction::RecoveryAp) {
            _apMode = true;
            _enterApMode(board);
            return currentPage;
        }

        const int64_t actionUtc = static_cast<int64_t>(time(nullptr));
        const uint32_t actionBatteryMv = board.readBatteryMv();
        const FocusClockConfig focusConfig = focusClockConfigFromAppConfig(cfg);
        if (isFocusDisplayPage(currentPage) &&
            applyFocusClockButtonAction(currentPage.managedPage(), action, focusConfig,
                                        _focusClockState, actionUtc)) {
            const auto chrome = buildChromeContext(cfg, actionUtc, localIP, actionBatteryMv);
            _renderDisplayPage(board, pageManager, currentPage, actionUtc, weather,
                               calendarSnapshot, cfg, localIP, chrome);
            const uint32_t contentHash = displayContentHash(currentPage, weather, cfg, localIP,
                                                            actionUtc, actionBatteryMv);
            gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, currentPage,
                                                             actionUtc, contentHash);
            lastActivityMs = millis();
            if (focusClockSessionIsActive(focusConfig, _focusClockState, actionUtc)) {
                currentPage = _runFocusClockSession(board, pageManager, currentPage, weather,
                                                    calendarSnapshot, cfg, localIP);
                lastActivityMs = millis();
            }
            continue;
        }

        if (isFocusDisplayPage(currentPage) &&
            focusClockBlocksButtonAction(currentPage.managedPage(), action, focusConfig,
                                         _focusClockState, actionUtc)) {
            log_i(TAG, "Focus Clock active: blocked button action %s",
                  buttonActionName(action));
            lastActivityMs = millis();
            continue;
        }

        const DisplayPageState selectedPage = applyButtonDisplayPageAction(pageManager, currentPage, action);
        if (action == ButtonAction::NextPage || action == ButtonAction::PreviousPage) {
            currentPage = selectedPage;
            const int64_t nowUtc = static_cast<int64_t>(time(nullptr));
            const uint32_t batteryMv = board.readBatteryMv();
            const auto chrome = buildChromeContext(cfg, nowUtc, localIP, batteryMv);
            _renderDisplayPage(board, pageManager, selectedPage, nowUtc, weather,
                               calendarSnapshot, cfg, localIP, chrome);
            const uint32_t contentHash = displayContentHash(selectedPage, weather, cfg, localIP,
                                                            nowUtc, batteryMv);
            gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, selectedPage, nowUtc, contentHash);
        }
        // Reset the inactivity budget AFTER processing so the render time does
        // not eat into the user's 30 s.
        lastActivityMs = millis();
    }

    return currentPage;
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2b-vii — Power-on config window (PortalSec > 0)
// ═════════════════════════════════════════════════════════════════════════════
// Keeps WiFi associated (modem sleep is enabled by the caller) and serves the
// web portal while buttons stay active. Button actions and web requests both
// refresh the inactivity budget; a hard cap bounds the total on-time.
DisplayPageState DashboardApp::_runConfigWindow(IBoard &board, PageManager &pageManager,
                                                DisplayPageState currentPage,
                                                WeatherClass &weather,
                                                const CalendarPageSnapshot &calendarSnapshot,
                                                const AppConfig &cfg,
                                                const String &localIP) {
    const uint8_t bootPin = board.bootButtonPin();
    const uint8_t userPin = board.apButtonPin();
    configureButtonInput(bootPin);
    configureButtonInput(userPin);

    WebServer webServer;
    webServer.start();
    log_i(TAG, "Config window: %u s — portal at http://%s/",
          static_cast<unsigned>(cfg.portalWindowSec), localIP.c_str());

    ButtonController buttons;
    const uint32_t budgetMs      = static_cast<uint32_t>(cfg.portalWindowSec) * 1000UL;
    const uint32_t hardCapMs     = 10UL * 60UL * 1000UL;
    const uint32_t windowStartMs = millis();
    uint32_t lastActivityMs      = windowStartMs;
    FocusHoldState focusHold;

    while (millis() - lastActivityMs < budgetMs &&
           millis() - windowStartMs < hardCapMs) {
        const uint32_t nowMs = millis();
        // Active browsing extends the window (wrap-safe comparison).
        const uint32_t webMs = webServer.lastActivityMs();
        if (static_cast<int32_t>(webMs - lastActivityMs) > 0) {
            lastActivityMs = webMs;
        }

        // Edge-based button polling: actions fire once, on release.
        ButtonAction action = ButtonAction::None;
        const bool userPressed = isPressedPin(userPin);
        action = pollFocusUserHold(currentPage, userPressed, nowMs, focusHold);
        if (action == ButtonAction::None && bootPin != 0xFF) {
            action = buttons.update(ButtonId::Boot, isPressedPin(bootPin), nowMs);
        }
        if (action == ButtonAction::None && userPin != 0xFF &&
            !isFocusDisplayPage(currentPage)) {
            action = buttons.update(ButtonId::User, userPressed, nowMs);
        }
        if (action == ButtonAction::None) {
            delay(20);
            continue;
        }

        _lastButtonAction = action;
        lastActivityMs = millis();
        log_i(TAG, "Config-window button action: %s", buttonActionName(action));

        if (action == ButtonAction::OpenConfig || action == ButtonAction::RecoveryAp) {
            webServer.stop();
            _apMode = true;
            _enterApMode(board);  // never returns
            return currentPage;
        }

        const int64_t actionUtc = static_cast<int64_t>(time(nullptr));
        const uint32_t actionBatteryMv = board.readBatteryMv();
        const FocusClockConfig focusConfig = focusClockConfigFromAppConfig(cfg);
        if (isFocusDisplayPage(currentPage) &&
            applyFocusClockButtonAction(currentPage.managedPage(), action, focusConfig,
                                        _focusClockState, actionUtc)) {
            const auto chrome = buildChromeContext(cfg, actionUtc, localIP, actionBatteryMv);
            _renderDisplayPage(board, pageManager, currentPage, actionUtc, weather,
                               calendarSnapshot, cfg, localIP, chrome);
            const uint32_t contentHash = displayContentHash(currentPage, weather, cfg, localIP,
                                                            actionUtc, actionBatteryMv);
            gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, currentPage,
                                                             actionUtc, contentHash);
            lastActivityMs = millis();
            if (focusClockSessionIsActive(focusConfig, _focusClockState, actionUtc)) {
                webServer.stop();
                currentPage = _runFocusClockSession(board, pageManager, currentPage, weather,
                                                    calendarSnapshot, cfg, localIP);
                return _runInteractiveWindow(board, pageManager, currentPage, weather,
                                             calendarSnapshot, cfg, localIP);
            }
            delay(20);
            continue;
        }

        if (isFocusDisplayPage(currentPage) &&
            focusClockBlocksButtonAction(currentPage.managedPage(), action, focusConfig,
                                         _focusClockState, actionUtc)) {
            log_i(TAG, "Focus Clock active: blocked config-window button action %s",
                  buttonActionName(action));
            delay(20);
            continue;
        }

        const DisplayPageState selectedPage = applyButtonDisplayPageAction(pageManager, currentPage, action);
        if (action == ButtonAction::NextPage || action == ButtonAction::PreviousPage) {
            currentPage = selectedPage;
            const int64_t nowUtc = static_cast<int64_t>(time(nullptr));
            const uint32_t batteryMv = board.readBatteryMv();
            const auto chrome = buildChromeContext(cfg, nowUtc, localIP, batteryMv);
            _renderDisplayPage(board, pageManager, selectedPage, nowUtc, weather,
                               calendarSnapshot, cfg, localIP, chrome);
            const uint32_t contentHash = displayContentHash(selectedPage, weather, cfg, localIP,
                                                            nowUtc, batteryMv);
            gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, selectedPage, nowUtc, contentHash);
            // Reset the budget AFTER rendering so refresh time is not billed
            // to the user's window.
            lastActivityMs = millis();
        }
        delay(20);
    }

    log_i(TAG, "Config window closed");
    webServer.stop();
    return currentPage;
}

void DashboardApp::_enterScheduledSleep(IBoard &board, uint64_t deepSleepUs,
                                        DisplayPageState currentPage) {
    const bool stored = savePersistedDisplayPage(currentPage);
    log_i(TAG, "Entering deep sleep: deepTimer=%llu us currentPage=%ld stored=%d",
          static_cast<unsigned long long>(deepSleepUs),
          static_cast<long>(storedValueForDisplayPage(currentPage)),
          stored);
    board.epd().hibernate();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    board.deepSleep(deepSleepUs);
}

// ═════════════════════════════════════════════════════════════════════════════
// Entry point
// ═════════════════════════════════════════════════════════════════════════════
void DashboardApp::run() {
    _detectWakeup(); // must be first — times button press before board.init()

    IBoard &board = getBoard();
    board.init();
    delay(kSerialSettleMs);

    log_i(TAG, "=== ESP32-Dashboard starting (cause=%d, cold=%d, fails=%d) ===",
          (int)esp_sleep_get_wakeup_cause(), _coldBoot, _failCount);
    const bool hasPsram = psramFound();
    const size_t psramBytes = ESP.getPsramSize();
    const CapacityProfile capacity = detectCapacity(hasPsram, psramBytes);
    log_i(TAG,
          "Memory: psram=%s size=%u freePsram=%u heap=%u maxAlloc=%u profile=%s slots=%u events=%u sourceMax=%u",
          hasPsram ? "yes" : "no",
          static_cast<unsigned>(psramBytes),
          static_cast<unsigned>(ESP.getFreePsram()),
          static_cast<unsigned>(ESP.getFreeHeap()),
          static_cast<unsigned>(ESP.getMaxAllocHeap()),
          capacity.extended ? "Extended" : "Safe",
          capacity.calendarSlots,
          capacity.maxExpandedEvents,
          static_cast<unsigned>(capacity.maxSourceBytes));

    AppConfig cfg;
    loadAppConfig(cfg);
    log_i(TAG, "Config: ssid=%s  lat=%s  lon=%s  utcOffset=%+d  lang=%s  sleep=%dmin",
          cfg.wifiSsid.c_str(), cfg.lat.c_str(), cfg.lon.c_str(),
          cfg.utcOffset, cfg.language.c_str(), cfg.sleepDuration);
    const PageSettings settings = pageSettingsFromConfig(cfg);
    PageManager pageManager(settings);
    const DisplayPageState persistedPage = loadPersistedDisplayPage(settings);
    DisplayPageState displayPage = selectStartupDisplayPage(persistedPage, settings, _restorePersistedPage);
    if (!displayPage.isHomeWeather()) {
        displayPage = DisplayPageState::managed(pageManager.restoreStoredPage(displayPage.managedPage()));
    } else {
        pageManager.restoreStoredPage(pageManager.firstPage());
    }
    log_i(TAG, "Startup page: %ld persisted=%ld restore=%d",
          static_cast<long>(storedValueForDisplayPage(displayPage)),
          static_cast<long>(storedValueForDisplayPage(persistedPage)),
          _restorePersistedPage ? 1 : 0);
    const PageSyncRequirements syncReq =
        syncRequirementsForDisplayPage(displayPage.isHomeWeather(), displayPage.managedPage());
    log_i(TAG, "Sync requirements: weather=%d calendar=%d finance=%d news=%d",
          syncReq.weather ? 1 : 0, syncReq.calendar ? 1 : 0,
          syncReq.finance ? 1 : 0, syncReq.news ? 1 : 0);

    _initHardware(board, _coldBoot);

    if (_apMode)   { _enterApMode(board); /* never returns */ }
    const bool forceInitialRender = _coldBoot;
    if (_coldBoot) {
        char splashBuf[64];
        snprintf(splashBuf, sizeof(splashBuf), "Connecting to\n%s...",
                 cfg.wifiSsid.c_str());
        _showLoadingPage(board, splashBuf);
    }
    _coldBoot = false;

    WifiManager wifi;
    if (!_connectAndSync(board, wifi, cfg)) {
        log_w(TAG, "WiFi failed — entering AP mode for recovery");
        _enterApMode(board); // never returns; restarts after kApTimeoutMs
        return;
    }

    WeatherClass weather;
    String localIP;
    bool dataOk = _fetchData(board, weather, cfg, localIP, syncReq.weather);

    if (!dataOk) {
        wifi.disconnect();
        log_i(TAG, "WiFi disconnected");
        log_i(TAG, "Sleeping for %d minutes", cfg.sleepDuration);
        _enterScheduledSleep(board,
                             static_cast<uint64_t>(cfg.sleepDuration) * 60ULL * 1000000ULL,
                             displayPage);
        return;
    }

    const RtcPageState previousRtcPageState = gDashboardRtcPageState;
    const PageId previousPage = previousRtcPageState.currentPage;
    const PageId selectedPage = renderCoordinatorPageId(displayPage);
    const int64_t nowUtc = static_cast<int64_t>(time(nullptr));
    const uint32_t batteryMv = board.readBatteryMv();
    const auto chrome = buildChromeContext(cfg, nowUtc, localIP, batteryMv);
    const CalendarPageSnapshot calendarSnapshot = syncReq.calendar
        ? syncCalendarPageSnapshot(cfg, nowUtc, capacity)
        : sampleCalendarPageSnapshot();
#if defined(UI_LAYOUT_EPD_400x300)
    gFinanceSnapshot = syncReq.finance
        ? syncFinancePageSnapshot(cfg, nowUtc, capacity)
        : sampleFinancePageSnapshot();
    gNewsSnapshot = syncReq.news
        ? syncNewsPageSnapshot(cfg, capacity)
        : sampleNewsPageSnapshot();
#endif
    const uint32_t contentHash = displayContentHash(displayPage, weather, cfg, localIP,
                                                    nowUtc, batteryMv);
    RenderInputs renderInput;
    renderInput.requestedPage = selectedPage;
    renderInput.currentPage = previousPage;
    renderInput.contentHash = contentHash;
    renderInput.lastContentHash = previousRtcPageState.lastContentHash;
    renderInput.nowUtc = nowUtc;
    renderInput.lastFullRefreshUtc = previousRtcPageState.lastFullRefreshUtc;
    renderInput.forceRefresh = forceInitialRender || _lastButtonAction != ButtonAction::None;

    RenderCoordinator renderCoordinator;
    const RenderDecision renderDecision = renderCoordinator.decide(renderInput);
    if (renderDecision.shouldRender) {
        _renderDisplayPage(board, pageManager, displayPage, nowUtc, weather, calendarSnapshot,
                           cfg, localIP, chrome);
        gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, displayPage, nowUtc, contentHash);
    } else {
        log_i(TAG, "Render skipped: page=%ld deferred=%d hash=0x%08lx lastHash=0x%08lx",
              static_cast<long>(storedValueForDisplayPage(displayPage)),
              renderDecision.deferred,
              static_cast<unsigned long>(contentHash),
              static_cast<unsigned long>(previousRtcPageState.lastContentHash));
        gDashboardRtcPageState = snapshotDisplayRtcState(pageManager, displayPage,
                                                         previousRtcPageState.lastFullRefreshUtc,
                                                         previousRtcPageState.lastContentHash);
    }
    DisplayPageState finalPage = displayPage;
    if (isFocusDisplayPage(displayPage) &&
        focusClockSessionIsActive(focusClockConfigFromAppConfig(cfg), _focusClockState,
                                  static_cast<int64_t>(time(nullptr)))) {
        finalPage = _runFocusClockSession(board, pageManager, displayPage, weather,
                                          calendarSnapshot, cfg, localIP);
        finalPage = _runInteractiveWindow(board, pageManager, finalPage, weather,
                                          calendarSnapshot, cfg, localIP);
    } else if (cfg.portalWindowSec > 0) {
        // Online profile: keep WiFi associated (modem sleep) and serve the
        // web portal for the configured window; buttons stay active too.
        WiFi.setSleep(true);
        finalPage = _runConfigWindow(board, pageManager, displayPage, weather, calendarSnapshot,
                                     cfg, localIP);
    } else {
        // Offline profile: radios off, awake button window until idle.
        wifi.disconnect();
        log_i(TAG, "WiFi disconnected");
        finalPage = _runInteractiveWindow(board, pageManager, displayPage, weather,
                                          calendarSnapshot, cfg, localIP);
    }
    log_i(TAG, "Sleeping for %d minutes", cfg.sleepDuration);
    _enterScheduledSleep(board,
                         static_cast<uint64_t>(cfg.sleepDuration) * 60ULL * 1000000ULL,
                         finalPage);
}
