#include "web_server.h"

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

#include "app/calendar/calendar_source.h"
#include "app/config/app_config.h"
#include "app/config/config_health.h"
#include "app/config/nvs_table.h"
#include "app/page/page_catalog.h"
#include "app/page/page_manager.h"
#include "app/security/secret_store.h"
#include "app/web/web_config_validation.h"
#include "utils/logger.h"

static const char *TAG_WS = "WebServer";

static AsyncWebServer _ws(80);
static bool _ws_started = false;
static constexpr size_t kMaxConfigPatchBytes = 8192;

static volatile uint32_t _lastRequestMs = 0;

static void touchActivity() {
    _lastRequestMs = millis();
}

static bool isSensitiveConfigKey(const String &key) {
    return redactKeyValueForLog(key.c_str(), "").find("redacted") != std::string::npos ||
           key == NVS_KEY_WIFI_PASSWORD ||
           key == NVS_KEY_PORTFOLIO_POS;
}

static String configToJson(const AppConfig &cfg) {
    JsonDocument doc;
    doc[NVS_KEY_WIFI_SSID]      = cfg.wifiSsid;
    doc[NVS_KEY_WIFI_PASSWORD]  = "";
    doc[NVS_KEY_LAT]            = cfg.lat;
    doc[NVS_KEY_LON]            = cfg.lon;
    doc[NVS_KEY_CITY]           = cfg.city;
    doc[NVS_KEY_TIMEZONE]       = cfg.utcOffset;
    doc[NVS_KEY_SLEEP_DURATION] = cfg.sleepDuration;
    doc[NVS_KEY_BED_TIME]       = cfg.bedTime;
    doc[NVS_KEY_WAKE_TIME]      = cfg.wakeTime;
    doc[NVS_KEY_PORTAL_WINDOW]  = cfg.portalWindowSec;
    doc[NVS_KEY_UNITS_TEMP]     = cfg.unitsTemp;
    doc[NVS_KEY_UNITS_SPEED]    = cfg.unitsSpeed;
    doc[NVS_KEY_UNITS_PRES]     = cfg.unitsPres;
    doc[NVS_KEY_UNITS_DIST]     = cfg.unitsDist;
    doc[NVS_KEY_UNITS_PRECIP]   = cfg.unitsPrecip;
    doc[NVS_KEY_TIME_FORMAT]    = cfg.timeFormat;
    doc[NVS_KEY_DATE_FORMAT]    = cfg.dateFormat;
    doc[NVS_KEY_LANGUAGE]       = cfg.language;
    doc[NVS_KEY_TIME_ZONE_ID]   = cfg.timeZoneId;
    doc[NVS_KEY_NEWS_FEEDS]     = cfg.newsFeeds;
    doc[NVS_KEY_STOCK_SYMBOLS]  = cfg.stockSymbols;
    doc[NVS_KEY_PORTFOLIO_POS]  = cfg.portfolioPositions.length() > 0 ? "<configured>" : "";
    doc[NVS_KEY_ECONOMIC_FEEDS] = cfg.economicFeeds;
    doc[NVS_KEY_WORLD_ZONES]    = cfg.worldClockZones;
    doc[NVS_KEY_FOCUS_LABEL]    = cfg.focusLabel;
    doc[NVS_KEY_FOCUS_MINUTES]  = cfg.focusMinutes;
    doc[NVS_KEY_FOCUS_BREAK_MIN]= cfg.focusBreakMinutes;
    doc[NVS_KEY_FOCUS_SESSIONS] = cfg.focusSessionCount;
    doc[NVS_KEY_INDOOR_ENABLED] = cfg.indoorSensorEnabled;
    doc[NVS_KEY_INDOOR_ROOM]    = cfg.indoorRoom;
    String out;
    serializeJson(doc, out);
    return out;
}

static PageSettings pageSettingsFromConfig(const AppConfig &cfg) {
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

static void copyPageSettingsToConfig(AppConfig &cfg, const PageSettings &settings) {
    const PageSettings clean = sanitizePageSettings(settings);
    cfg.configVersion = clean.configVersion;
    cfg.pageEnabledMask = clean.enabledMask;
    cfg.pageAutoRotateMask = clean.autoRotateMask;
    cfg.pageOrderCount = std::min(clean.orderCount, static_cast<size_t>(APP_CONFIG_PAGE_ORDER_MAX));
    for (size_t i = 0; i < cfg.pageOrderCount; ++i) {
        cfg.pageOrder[i] = static_cast<uint8_t>(clean.order[i]);
    }
    cfg.pageTemplateId = static_cast<uint8_t>(clean.templateId);
    cfg.rotationIntervalMinutes = clean.rotationIntervalMinutes;
    cfg.timeZoneId = clean.timeZoneId.c_str();
}

static const char *pageCategoryName(PageCategory category) {
    switch (category) {
        case PageCategory::Calendar: return "Calendar";
        case PageCategory::Weather: return "Weather";
        case PageCategory::Time: return "Time";
        case PageCategory::Finance: return "Finance";
        case PageCategory::News: return "News";
        case PageCategory::Notes: return "Notes";
    }
    return "Other";
}

static const char *pagePriorityName(PagePriority priority) {
    switch (priority) {
        case PagePriority::P0: return "P0";
        case PagePriority::P1: return "P1";
        case PagePriority::P2: return "P2";
    }
    return "P2";
}

static bool jsonPageId(JsonVariantConst value, PageId &out) {
    if (value.isNull()) {
        return false;
    }
    const int raw = value.as<int>();
    if (raw < 0 || raw >= static_cast<int>(kPageCount)) {
        return false;
    }
    out = static_cast<PageId>(raw);
    return true;
}

static SourceConfigSummary sourceSummaryFromRuntime(const AppConfig &cfg) {
    SourceConfigSummary summary = sourceSummaryFromConfig(cfg);
    NvsSecretBackend backend;
    CalendarSecretStore calendarStore(backend);
    for (uint8_t i = 0; i < CALENDAR_SOURCE_MAX_COUNT; ++i) {
        const CalendarSourceMetadata meta = calendarStore.readMetadata(i);
        if (meta.configured) {
            ++summary.configuredCalendarSources;
            if (meta.enabled) {
                ++summary.enabledCalendarSources;
            }
        }
    }
    return summary;
}

static void sendJson(AsyncWebServerRequest *req, JsonDocument &doc, int status = 200) {
    String out;
    serializeJson(doc, out);
    req->send(status, "application/json", out);
}

static void appendReadinessJson(JsonObject obj, const PageReadiness *readiness) {
    if (!readiness) {
        obj["state"] = "Error";
        obj["dependency"] = "page";
        obj["message"] = "No readiness data";
        return;
    }
    obj["state"] = pageReadinessStateName(readiness->state);
    obj["dependency"] = readiness->dependency.c_str();
    obj["message"] = readiness->message.c_str();
}

static void sendPagesResponse(AsyncWebServerRequest *req) {
    AppConfig cfg;
    loadAppConfig(cfg);
    const PageSettings settings = pageSettingsFromConfig(cfg);
    const SourceConfigSummary sources = sourceSummaryFromRuntime(cfg);
    const ConfigHealth health = buildConfigHealth(cfg, settings, sources);

    JsonDocument doc;
    doc["totalPages"] = health.totalDisplayPages;
    doc["enabledManagedPages"] = health.enabledManagedPages;
    doc["allReady"] = health.allEnabledPagesReady;
    doc["requiredCount"] = health.requiredCount;
    doc["optionalCount"] = health.optionalCount;
    doc["errorCount"] = health.errorCount;
    doc["rotationIntervalMinutes"] = settings.rotationIntervalMinutes;

    JsonObject home = doc["home"].to<JsonObject>();
    home["id"] = -1;
    home["name"] = "Weather Home";
    home["enabled"] = true;
    home["autoRotate"] = false;
    home["order"] = 0;
    home["pageNumber"] = 0;
    JsonObject homeReady = home["readiness"].to<JsonObject>();
    homeReady["state"] = "Ready";
    homeReady["dependency"] = "weather";
    homeReady["message"] = "Legacy PageWeather home is always enabled";

    JsonArray pages = doc["pages"].to<JsonArray>();
    for (const PageDescriptor &descriptor : pageCatalog()) {
        JsonObject obj = pages.add<JsonObject>();
        obj["id"] = static_cast<int>(descriptor.id);
        obj["name"] = descriptor.name;
        obj["category"] = pageCategoryName(descriptor.category);
        obj["priority"] = pagePriorityName(descriptor.priority);
        obj["enabled"] = (settings.enabledMask & pageMask(descriptor.id)) != 0;
        obj["autoRotate"] = (settings.autoRotateMask & pageMask(descriptor.id)) != 0;
        obj["pageNumber"] = obj["enabled"] ? PageManager(settings).pageNumber(descriptor.id) + 1 : 0;

        size_t orderIndex = 0;
        for (size_t i = 0; i < settings.orderCount; ++i) {
            if (settings.order[i] == descriptor.id) {
                orderIndex = i + 1;
                break;
            }
        }
        obj["order"] = orderIndex;
        JsonArray providers = obj["providers"].to<JsonArray>();
        for (const std::string &provider : descriptor.requiredProviders) {
            providers.add(provider.c_str());
        }
        JsonObject readiness = obj["readiness"].to<JsonObject>();
        appendReadinessJson(readiness, findPageReadiness(health, descriptor.id));
    }

    sendJson(req, doc);
}

static bool applyPagesPatch(const String &body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        return false;
    }
    AppConfig cfg;
    loadAppConfig(cfg);
    PageSettings settings = pageSettingsFromConfig(cfg);

    JsonArrayConst enabled = doc["enabled"].as<JsonArrayConst>();
    if (!enabled.isNull()) {
        uint32_t mask = 0;
        for (JsonVariantConst v : enabled) {
            PageId page;
            if (jsonPageId(v, page)) {
                mask |= pageMask(page);
            }
        }
        settings.enabledMask = mask;
    }

    JsonArrayConst autoRotate = doc["autoRotate"].as<JsonArrayConst>();
    if (!autoRotate.isNull()) {
        uint32_t mask = 0;
        for (JsonVariantConst v : autoRotate) {
            PageId page;
            if (jsonPageId(v, page)) {
                mask |= pageMask(page);
            }
        }
        settings.autoRotateMask = mask;
    }

    JsonArrayConst order = doc["order"].as<JsonArrayConst>();
    if (!order.isNull()) {
        settings.orderCount = 0;
        for (JsonVariantConst v : order) {
            PageId page;
            if (jsonPageId(v, page) && settings.orderCount < settings.order.size()) {
                settings.order[settings.orderCount++] = page;
            }
        }
    }

    JsonVariantConst rotateVar = doc["rotationIntervalMinutes"];
    if (!rotateVar.isNull()) {
        settings.rotationIntervalMinutes = static_cast<uint16_t>(rotateVar.as<int>());
    }

    copyPageSettingsToConfig(cfg, settings);
    saveAppConfig(cfg);
    return true;
}

static void sendHealthResponse(AsyncWebServerRequest *req) {
    AppConfig cfg;
    loadAppConfig(cfg);
    const PageSettings settings = pageSettingsFromConfig(cfg);
    const SourceConfigSummary sources = sourceSummaryFromRuntime(cfg);
    const ConfigHealth health = buildConfigHealth(cfg, settings, sources);

    JsonDocument doc;
    doc["allReady"] = health.allEnabledPagesReady;
    doc["totalPages"] = health.totalDisplayPages;
    doc["enabledManagedPages"] = health.enabledManagedPages;
    doc["requiredCount"] = health.requiredCount;
    doc["optionalCount"] = health.optionalCount;
    doc["errorCount"] = health.errorCount;
    JsonArray pages = doc["pages"].to<JsonArray>();
    for (const PageReadiness &readiness : health.pages) {
        JsonObject obj = pages.add<JsonObject>();
        obj["id"] = static_cast<int>(readiness.page);
        const PageDescriptor *descriptor = findPage(readiness.page);
        obj["name"] = descriptor ? descriptor->name : "Unknown";
        obj["enabled"] = readiness.enabled;
        obj["state"] = pageReadinessStateName(readiness.state);
        obj["dependency"] = readiness.dependency.c_str();
        obj["message"] = readiness.message.c_str();
    }
    sendJson(req, doc);
}

static void sendSourceResponse(AsyncWebServerRequest *req, const char *source) {
    AppConfig cfg;
    loadAppConfig(cfg);
    JsonDocument doc;
    doc["source"] = source;

    if (strcmp(source, "weather") == 0) {
        doc[NVS_KEY_CITY] = cfg.city;
        doc[NVS_KEY_LAT] = cfg.lat;
        doc[NVS_KEY_LON] = cfg.lon;
        doc[NVS_KEY_UNITS_TEMP] = cfg.unitsTemp;
        doc[NVS_KEY_UNITS_SPEED] = cfg.unitsSpeed;
        doc[NVS_KEY_UNITS_PRECIP] = cfg.unitsPrecip;
    } else if (strcmp(source, "calendar") == 0) {
        NvsSecretBackend backend;
        CalendarSecretStore store(backend);
        JsonArray sources = doc["sources"].to<JsonArray>();
        for (uint8_t i = 0; i < CALENDAR_SOURCE_MAX_COUNT; ++i) {
            const CalendarSourceMetadata meta = store.readMetadata(i);
            if (!meta.configured) {
                continue;
            }
            JsonObject obj = sources.add<JsonObject>();
            obj["index"] = meta.index;
            obj["enabled"] = meta.enabled;
            obj["color"] = meta.color;
            obj["alias"] = meta.alias.c_str();
            obj["host"] = meta.host.c_str();
            obj["maskedUrl"] = meta.maskedUrl.c_str();
            obj["maskedApiKey"] = meta.maskedApiKey.c_str();
        }
    } else if (strcmp(source, "news") == 0) {
        doc["feeds"] = cfg.newsFeeds;
    } else if (strcmp(source, "finance") == 0) {
        doc["symbols"] = cfg.stockSymbols;
    } else if (strcmp(source, "portfolio") == 0) {
        SourceConfigSummary summary = sourceSummaryFromConfig(cfg);
        doc["configured"] = summary.portfolioPositions > 0;
        doc["positionCount"] = summary.portfolioPositions;
    } else if (strcmp(source, "economic") == 0) {
        doc["feeds"] = cfg.economicFeeds;
    } else if (strcmp(source, "time") == 0) {
        doc["zones"] = cfg.worldClockZones;
        doc[NVS_KEY_TIME_ZONE_ID] = cfg.timeZoneId;
    } else if (strcmp(source, "focus") == 0) {
        doc["label"] = cfg.focusLabel;
        doc["focusMinutes"] = cfg.focusMinutes;
        doc["breakMinutes"] = cfg.focusBreakMinutes;
        doc["sessionCount"] = cfg.focusSessionCount;
    } else if (strcmp(source, "indoor") == 0) {
        doc["enabled"] = cfg.indoorSensorEnabled;
        doc["room"] = cfg.indoorRoom;
    } else {
        doc["error"] = "unknown source";
        sendJson(req, doc, 404);
        return;
    }

    SourceConfigSummary summary = sourceSummaryFromRuntime(cfg);
    doc["summary"]["calendarEnabled"] = summary.enabledCalendarSources;
    doc["summary"]["newsFeeds"] = summary.newsFeeds;
    doc["summary"]["stockSymbols"] = summary.stockSymbols;
    doc["summary"]["portfolioPositions"] = summary.portfolioPositions;
    doc["summary"]["economicFeeds"] = summary.economicFeeds;
    doc["summary"]["indoorEnabled"] = summary.indoorEnabled;
    sendJson(req, doc);
}

static bool saveCalendarSourcePatch(JsonObjectConst obj) {
    CalendarSourceSecrets source;
    source.index = static_cast<uint8_t>(obj["index"] | 0);
    NvsSecretBackend backend;
    CalendarSecretStore store(backend);
    if (obj["delete"] | false) {
        return store.deleteSource(source.index);
    }
    CalendarSourceSecrets existing;
    const bool hasExisting = store.loadSourceForDownload(source.index, existing);
    source.url = std::string(obj["url"] | "");
    source.apiKey = std::string(obj["apiKey"] | "");
    if (source.url.empty() && hasExisting) {
        source.url = existing.url;
    }
    if (source.apiKey.empty() && hasExisting) {
        source.apiKey = existing.apiKey;
    }
    source.alias = std::string(obj["alias"] | "");
    if (source.alias.empty() && hasExisting) {
        source.alias = existing.alias;
    }
    source.enabled = obj["enabled"] | true;
    source.color = static_cast<uint8_t>(obj["color"] | 0);
    return store.saveSource(source);
}

static bool applySourcePatch(const char *source, const String &body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        return false;
    }

    if (strcmp(source, "calendar") == 0) {
        JsonArrayConst sources = doc["sources"].as<JsonArrayConst>();
        if (!sources.isNull()) {
            bool ok = true;
            for (JsonObjectConst obj : sources) {
                ok = saveCalendarSourcePatch(obj) && ok;
            }
            return ok;
        }
        return saveCalendarSourcePatch(doc.as<JsonObjectConst>());
    }

    AppConfig cfg;
    loadAppConfig(cfg);
    JsonObjectConst obj = doc.as<JsonObjectConst>();
    auto putStr = [&](const char *jsonKey, String &field) {
        JsonVariantConst v = obj[jsonKey];
        if (!v.isNull()) {
            field = v.as<String>();
        }
    };
    auto putBool = [&](const char *jsonKey, bool &field) {
        JsonVariantConst v = obj[jsonKey];
        if (!v.isNull()) {
            field = v.as<bool>();
        }
    };
    auto putU16 = [&](const char *jsonKey, uint16_t &field) {
        JsonVariantConst v = obj[jsonKey];
        if (!v.isNull()) {
            const int value = v.as<int>();
            field = static_cast<uint16_t>(value < 0 ? 0 : value);
        }
    };
    auto putU8 = [&](const char *jsonKey, uint8_t &field) {
        JsonVariantConst v = obj[jsonKey];
        if (!v.isNull()) {
            const int value = v.as<int>();
            field = static_cast<uint8_t>(value < 0 ? 0 : value);
        }
    };

    if (strcmp(source, "weather") == 0) {
        putStr(NVS_KEY_CITY, cfg.city);
        putStr(NVS_KEY_LAT, cfg.lat);
        putStr(NVS_KEY_LON, cfg.lon);
        putStr(NVS_KEY_UNITS_TEMP, cfg.unitsTemp);
        putStr(NVS_KEY_UNITS_SPEED, cfg.unitsSpeed);
        putStr(NVS_KEY_UNITS_PRECIP, cfg.unitsPrecip);
    } else if (strcmp(source, "news") == 0) {
        putStr("feeds", cfg.newsFeeds);
    } else if (strcmp(source, "finance") == 0) {
        putStr("symbols", cfg.stockSymbols);
    } else if (strcmp(source, "portfolio") == 0) {
        putStr("positions", cfg.portfolioPositions);
    } else if (strcmp(source, "economic") == 0) {
        putStr("feeds", cfg.economicFeeds);
    } else if (strcmp(source, "time") == 0) {
        putStr("zones", cfg.worldClockZones);
        putStr(NVS_KEY_TIME_ZONE_ID, cfg.timeZoneId);
    } else if (strcmp(source, "focus") == 0) {
        putStr("label", cfg.focusLabel);
        putU16("focusMinutes", cfg.focusMinutes);
        putU16("breakMinutes", cfg.focusBreakMinutes);
        putU8("sessionCount", cfg.focusSessionCount);
    } else if (strcmp(source, "indoor") == 0) {
        putBool("enabled", cfg.indoorSensorEnabled);
        putStr("room", cfg.indoorRoom);
    } else {
        return false;
    }

    saveAppConfig(cfg);
    log_w(TAG_WS, "Source config patched: %s", source);
    return true;
}

static bool applyPatch(const String &body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        log_e(TAG_WS, "JSON parse error in PATCH body");
        return false;
    }

    AppConfig cfg;
    loadAppConfig(cfg);
    JsonObjectConst obj = doc.as<JsonObjectConst>();

    auto tryStr = [&](const char *key, String &field) {
        JsonVariantConst v = obj[key];
        if (!v.isNull()) {
            String vStr = v.as<String>();
            if (vStr.length() > 0) field = vStr;
        }
    };
    auto tryInt = [&](const char *key, int &field) {
        JsonVariantConst v = obj[key];
        if (!v.isNull()) {
            field = v.as<int>();
        }
    };
    auto tryU16 = [&](const char *key, uint16_t &field) {
        JsonVariantConst v = obj[key];
        if (!v.isNull()) {
            const int value = v.as<int>();
            field = static_cast<uint16_t>(value < 0 ? 0 : value);
        }
    };
    auto tryU8 = [&](const char *key, uint8_t &field) {
        JsonVariantConst v = obj[key];
        if (!v.isNull()) {
            const int value = v.as<int>();
            field = static_cast<uint8_t>(value < 0 ? 0 : value);
        }
    };

    tryStr(NVS_KEY_WIFI_SSID,     cfg.wifiSsid);
    {
        JsonVariantConst pwdVar = obj[NVS_KEY_WIFI_PASSWORD];
        if (!pwdVar.isNull()) {
            String pwd = pwdVar.as<String>();
            if (pwd.length() > 0) cfg.wifiPassword = pwd;
        }
    }
    tryStr(NVS_KEY_LAT,           cfg.lat);
    tryStr(NVS_KEY_LON,           cfg.lon);
    tryStr(NVS_KEY_CITY,          cfg.city);
    tryInt(NVS_KEY_TIMEZONE,      cfg.utcOffset);
    tryInt(NVS_KEY_SLEEP_DURATION,cfg.sleepDuration);
    tryInt(NVS_KEY_BED_TIME,      cfg.bedTime);
    tryInt(NVS_KEY_WAKE_TIME,     cfg.wakeTime);
    {
        JsonVariantConst portalVar = obj[NVS_KEY_PORTAL_WINDOW];
        if (!portalVar.isNull()) {
            cfg.portalWindowSec = normalizePortalWindowSec(portalVar.as<int32_t>());
        }
    }
    tryStr(NVS_KEY_UNITS_TEMP,    cfg.unitsTemp);
    tryStr(NVS_KEY_UNITS_SPEED,   cfg.unitsSpeed);
    tryStr(NVS_KEY_UNITS_PRES,    cfg.unitsPres);
    tryStr(NVS_KEY_UNITS_DIST,    cfg.unitsDist);
    tryStr(NVS_KEY_UNITS_PRECIP,  cfg.unitsPrecip);
    tryStr(NVS_KEY_TIME_FORMAT,   cfg.timeFormat);
    tryStr(NVS_KEY_DATE_FORMAT,   cfg.dateFormat);
    tryStr(NVS_KEY_LANGUAGE,      cfg.language);
    tryStr(NVS_KEY_TIME_ZONE_ID,  cfg.timeZoneId);
    tryStr(NVS_KEY_NEWS_FEEDS,    cfg.newsFeeds);
    tryStr(NVS_KEY_STOCK_SYMBOLS, cfg.stockSymbols);
    tryStr(NVS_KEY_PORTFOLIO_POS, cfg.portfolioPositions);
    tryStr(NVS_KEY_ECONOMIC_FEEDS,cfg.economicFeeds);
    tryStr(NVS_KEY_WORLD_ZONES,   cfg.worldClockZones);
    tryStr(NVS_KEY_FOCUS_LABEL,   cfg.focusLabel);
    tryU16(NVS_KEY_FOCUS_MINUTES, cfg.focusMinutes);
    tryU16(NVS_KEY_FOCUS_BREAK_MIN, cfg.focusBreakMinutes);
    tryU8(NVS_KEY_FOCUS_SESSIONS, cfg.focusSessionCount);
    tryStr(NVS_KEY_INDOOR_ROOM,   cfg.indoorRoom);
    {
        JsonVariantConst indoorVar = obj[NVS_KEY_INDOOR_ENABLED];
        if (!indoorVar.isNull()) {
            cfg.indoorSensorEnabled = indoorVar.as<bool>();
        }
    }

    saveAppConfig(cfg);
    log_w(TAG_WS, "Config patched and saved to NVS:");
    for (JsonPairConst kv : doc.as<JsonObjectConst>()) {
        const String key = kv.key().c_str();
        log_w(TAG_WS, "  %s = %s", key.c_str(), isSensitiveConfigKey(key) ? "<redacted>" : "<updated>");
    }
    return true;
}

static void collectBody(AsyncWebServerRequest *req, uint8_t *data, size_t len,
                        size_t index, size_t total) {
    if (index == 0) {
        if (total > kMaxConfigPatchBytes) {
            return;
        }
        req->_tempObject = new String();
    }
    String *body = reinterpret_cast<String *>(req->_tempObject);
    if (body) {
        body->concat(reinterpret_cast<const char *>(data), len);
    }
}

static String consumeBody(AsyncWebServerRequest *req) {
    String *body = reinterpret_cast<String *>(req->_tempObject);
    String out = body ? *body : "";
    delete body;
    req->_tempObject = nullptr;
    return out;
}

static void registerSourceRoutes(const char *source) {
    String base = String("/api/sources/") + source;
    _ws.on(base.c_str(), HTTP_GET, [source](AsyncWebServerRequest *req) {
        touchActivity();
        sendSourceResponse(req, source);
    });
    _ws.on(base.c_str(), HTTP_POST,
        [source](AsyncWebServerRequest *req) {
            touchActivity();
            const String body = consumeBody(req);
            const bool ok = body.length() > 0 && applySourcePatch(source, body);
            req->send(ok ? 200 : 400, "application/json",
                      ok ? "{\"ok\":true}" : "{\"error\":\"invalid source config\"}");
        },
        nullptr,
        collectBody);

    String testPath = base + "/test";
    _ws.on(testPath.c_str(), HTTP_POST, [source](AsyncWebServerRequest *req) {
        touchActivity();
        AppConfig cfg;
        loadAppConfig(cfg);
        const SourceConfigSummary summary = sourceSummaryFromRuntime(cfg);
        JsonDocument doc;
        bool ok = true;
        const char *state = "Ready";
        const char *message = "Configuration accepted. Live fetch is handled on the next sync cycle.";
        if (strcmp(source, "calendar") == 0) {
            ok = summary.enabledCalendarSources > 0;
            state = ok ? "Ready" : "Required";
            message = ok ? "Calendar metadata is stored. URL/API key are masked in GET APIs."
                         : "Add at least one enabled ICS calendar source.";
        } else if (strcmp(source, "news") == 0) {
            ok = summary.newsFeeds > 0;
            state = ok ? "Ready" : "Required";
            message = ok ? "RSS/Atom feeds are configured. Defaults include BBC, Hacker News, and NASA."
                         : "Add at least one RSS 2.0 or Atom feed.";
        } else if (strcmp(source, "finance") == 0) {
            ok = summary.stockSymbols > 0;
            state = ok ? "Ready" : "Required";
            message = ok ? "Watchlist symbols are configured for Stooq CSV sync."
                         : "Add at least one Stooq symbol such as AAPL.US.";
        } else if (strcmp(source, "portfolio") == 0) {
            ok = summary.portfolioPositions > 0;
            state = ok ? "Ready" : "Required";
            message = ok ? "Portfolio positions are stored locally and not returned by GET APIs."
                         : "Add positions, for example AAPL:2:180:USD.";
        } else if (strcmp(source, "economic") == 0) {
            ok = summary.economicFeeds > 0;
            state = ok ? "Ready" : "Required";
            message = ok ? "Economic RSS/ICS feed URLs are configured."
                         : "Add a real RSS/ICS feed URL. Region labels such as US, EU are not live sources.";
        } else if (strcmp(source, "indoor") == 0) {
            ok = summary.indoorEnabled;
            state = ok ? "Ready" : "Optional";
            message = ok ? "Indoor sensor source is enabled."
                         : "Indoor source is optional unless the page is enabled and a sensor is installed.";
        }
        doc["ok"] = ok;
        doc["source"] = source;
        doc["state"] = state;
        doc["message"] = message;
        sendJson(req, doc);
    });
}

void WebServer::start() {
    if (_ws_started) return;

    if (!LittleFS.begin(false, "/littlefs", 3, kWebAssetsPartitionLabel)) {
        log_e(TAG_WS, "LittleFS mount failed - web assets unavailable");
    } else {
        log_i(TAG_WS, "LittleFS mounted: %u / %u bytes used",
              LittleFS.usedBytes(), LittleFS.totalBytes());
    }

    auto serveGz = [](AsyncWebServerRequest *req, const char *path, const char *mime) {
        if (!LittleFS.exists(path)) {
            req->send(503, "text/plain",
                      "Web assets not uploaded. Run: pio run -e nm-display-420 -t upload_all");
            return;
        }
        AsyncWebServerResponse *resp = req->beginResponse(LittleFS, path, mime);
        resp->addHeader("Content-Encoding", "gzip");
        resp->addHeader("Cache-Control", "no-cache");
        req->send(resp);
    };

    _ws.on("/", HTTP_GET, [serveGz](AsyncWebServerRequest *req) {
        touchActivity();
        serveGz(req, "/index.html.gz", "text/html");
    });

    _ws.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *req) {
        touchActivity();
        AppConfig cfg;
        loadAppConfig(cfg);
        req->send(200, "application/json", configToJson(cfg));
    });

    _ws.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest *req) {
            touchActivity();
            if (req->contentLength() > kMaxConfigPatchBytes) {
                consumeBody(req);
                req->send(413, "application/json", "{\"error\":\"body too large\"}");
                return;
            }
            const String body = consumeBody(req);
            if (body.isEmpty()) {
                req->send(400, "application/json", "{\"error\":\"empty body\"}");
                return;
            }
            const bool ok = applyPatch(body);
            req->send(ok ? 200 : 400, "application/json",
                      ok ? "{\"ok\":true}" : "{\"error\":\"invalid JSON\"}");
        },
        nullptr,
        collectBody);

    _ws.on("/api/pages", HTTP_GET, [](AsyncWebServerRequest *req) {
        touchActivity();
        sendPagesResponse(req);
    });

    _ws.on("/api/pages", HTTP_POST,
        [](AsyncWebServerRequest *req) {
            touchActivity();
            const String body = consumeBody(req);
            const bool ok = body.length() > 0 && applyPagesPatch(body);
            req->send(ok ? 200 : 400, "application/json",
                      ok ? "{\"ok\":true}" : "{\"error\":\"invalid pages config\"}");
        },
        nullptr,
        collectBody);

    _ws.on("/api/health", HTTP_GET, [](AsyncWebServerRequest *req) {
        touchActivity();
        sendHealthResponse(req);
    });

    registerSourceRoutes("weather");
    registerSourceRoutes("calendar");
    registerSourceRoutes("news");
    registerSourceRoutes("finance");
    registerSourceRoutes("portfolio");
    registerSourceRoutes("economic");
    registerSourceRoutes("time");
    registerSourceRoutes("focus");
    registerSourceRoutes("indoor");

    _ws.on("/api/sync", HTTP_POST, [](AsyncWebServerRequest *req) {
        touchActivity();
        JsonDocument doc;
        doc["ok"] = true;
        doc["state"] = "Queued";
        doc["message"] = "Enabled page dependencies will sync on the next wake cycle.";
        sendJson(req, doc);
    });

    _ws.on("/api/system/restart", HTTP_POST, [](AsyncWebServerRequest *req) {
        touchActivity();
        req->send(200, "application/json", "{\"ok\":true}");
        delay(200);
        ESP.restart();
    });

    _ws.onNotFound([](AsyncWebServerRequest *req) {
        touchActivity();
        req->send(404, "text/plain", "Not found");
    });

    _ws.begin();
    _ws_started = true;
    _lastRequestMs = millis();
    IPAddress serverIP = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
                         ? WiFi.softAPIP() : WiFi.localIP();
    log_i(TAG_WS, "HTTP server started -> http://%s/", serverIP.toString().c_str());
}

void WebServer::stop() {
    if (!_ws_started) return;
    _ws.end();
    _ws_started = false;
    log_i(TAG_WS, "HTTP server stopped");
}

uint32_t WebServer::lastActivityMs() const {
    return _lastRequestMs;
}
