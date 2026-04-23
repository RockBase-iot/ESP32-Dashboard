#include "web_server.h"

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "app/config/app_config.h"
#include "app/config/nvs_table.h"
#include "utils/logger.h"

static const char *TAG_WS = "WebServer";

static AsyncWebServer _ws(80);
static bool _ws_started = false;

// ── Serialise AppConfig → JSON ───────────────────────────────────────────────
static String configToJson(const AppConfig &cfg) {
    JsonDocument doc;
    doc[NVS_KEY_WIFI_SSID]      = cfg.wifiSsid;
    doc[NVS_KEY_WIFI_PASSWORD]  = "";          // never send real password back
    doc[NVS_KEY_LAT]            = cfg.lat;
    doc[NVS_KEY_LON]            = cfg.lon;
    doc[NVS_KEY_CITY]           = cfg.city;
    doc[NVS_KEY_TIMEZONE]       = cfg.timezone;
    doc[NVS_KEY_SLEEP_DURATION] = cfg.sleepDuration;
    doc[NVS_KEY_BED_TIME]       = cfg.bedTime;
    doc[NVS_KEY_WAKE_TIME]      = cfg.wakeTime;
    doc[NVS_KEY_UNITS_TEMP]     = cfg.unitsTemp;
    doc[NVS_KEY_UNITS_SPEED]    = cfg.unitsSpeed;
    doc[NVS_KEY_UNITS_PRES]     = cfg.unitsPres;
    doc[NVS_KEY_UNITS_DIST]     = cfg.unitsDist;
    doc[NVS_KEY_UNITS_PRECIP]   = cfg.unitsPrecip;
    doc[NVS_KEY_TIME_FORMAT]    = cfg.timeFormat;
    doc[NVS_KEY_DATE_FORMAT]    = cfg.dateFormat;
    doc[NVS_KEY_LANGUAGE]       = cfg.language;
    String out;
    serializeJson(doc, out);
    return out;
}

// ── Apply JSON patch → AppConfig, then persist ───────────────────────────────
static bool applyPatch(const String &body) {
    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        log_e(TAG_WS, "JSON parse error in PATCH body");
        return false;
    }

    // Load current values so unspecified fields keep their value
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

    tryStr(NVS_KEY_WIFI_SSID,     cfg.wifiSsid);
    // Only update password if a non-empty value was explicitly sent
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
    tryStr(NVS_KEY_TIMEZONE,      cfg.timezone);
    tryInt(NVS_KEY_SLEEP_DURATION,cfg.sleepDuration);
    tryInt(NVS_KEY_BED_TIME,      cfg.bedTime);
    tryInt(NVS_KEY_WAKE_TIME,     cfg.wakeTime);
    tryStr(NVS_KEY_UNITS_TEMP,    cfg.unitsTemp);
    tryStr(NVS_KEY_UNITS_SPEED,   cfg.unitsSpeed);
    tryStr(NVS_KEY_UNITS_PRES,    cfg.unitsPres);
    tryStr(NVS_KEY_UNITS_DIST,    cfg.unitsDist);
    tryStr(NVS_KEY_UNITS_PRECIP,  cfg.unitsPrecip);
    tryStr(NVS_KEY_TIME_FORMAT,   cfg.timeFormat);
    tryStr(NVS_KEY_DATE_FORMAT,   cfg.dateFormat);
    tryStr(NVS_KEY_LANGUAGE,      cfg.language);

    saveAppConfig(cfg);
    log_w(TAG_WS, "Config patched and saved to NVS:");
    for (JsonPair kv : doc.as<JsonObject>()) {
        log_w(TAG_WS, "  %s = %s", kv.key().c_str(), kv.value().as<String>().c_str());
    }
    return true;
}

// ── Body collector: shared onBody handler for POST routes ──────────────────
static void collectBody(AsyncWebServerRequest *req, uint8_t *data, size_t len,
                        size_t index, size_t /*total*/) {
    if (index == 0) req->_tempObject = new String();
    reinterpret_cast<String *>(req->_tempObject)->concat(
        reinterpret_cast<const char *>(data), len);
}

// ────────────────────────────────────────────────────────────────────────────
void WebServer::start() {
    if (_ws_started) return;

    // ── Mount LittleFS ───────────────────────────────────────────────────────
    if (!LittleFS.begin(false, "/littlefs", 3)) {
        log_e(TAG_WS, "LittleFS mount failed — web assets unavailable");
    } else {
        log_i(TAG_WS, "LittleFS mounted: %u / %u bytes used",
              LittleFS.usedBytes(), LittleFS.totalBytes());
    }

    // ── Helper: serve a gzipped file from LittleFS ───────────────────────────
    auto serveGz = [](AsyncWebServerRequest *req, const char *path, const char *mime) {
        if (!LittleFS.exists(path)) {
            req->send(503, "text/plain", "Web assets not uploaded. Run: pio run -t uploadfs");
            return;
        }
        AsyncWebServerResponse *resp = req->beginResponse(LittleFS, path, mime);
        resp->addHeader("Content-Encoding", "gzip");
        resp->addHeader("Cache-Control", "no-cache");
        req->send(resp);
    };

    // ── GET / → index.html.gz ────────────────────────────────────────────────
    _ws.on("/", HTTP_GET, [serveGz](AsyncWebServerRequest *req) {
        serveGz(req, "/index.html.gz", "text/html");
    });

    // ── GET /api/config → full config JSON (password redacted) ──────────────
    _ws.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *req) {
        AppConfig cfg;
        loadAppConfig(cfg);
        req->send(200, "application/json", configToJson(cfg));
    });

    // ── POST /api/config → JSON patch ────────────────────────────────────────
    _ws.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest *req) {
            String *body = reinterpret_cast<String *>(req->_tempObject);
            if (!body || body->isEmpty()) {
                req->send(400, "application/json", "{\"error\":\"empty body\"}");
                return;
            }
            bool ok = applyPatch(*body);
            delete body;
            req->_tempObject = nullptr;
            req->send(ok ? 200 : 400, "application/json",
                      ok ? "{\"ok\":true}" : "{\"error\":\"invalid JSON\"}");
        },
        nullptr,       // onUpload
        collectBody
    );

    // ── POST /api/system/restart ──────────────────────────────────────────────
    _ws.on("/api/system/restart", HTTP_POST, [](AsyncWebServerRequest *req) {
        req->send(200, "application/json", "{\"ok\":true}");
        delay(200);
        ESP.restart();
    });

    // ── 404 fallback ──────────────────────────────────────────────────────────
    _ws.onNotFound([](AsyncWebServerRequest *req) {
        req->send(404, "text/plain", "Not found");
    });

    _ws.begin();
    _ws_started = true;
    IPAddress serverIP = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)
                         ? WiFi.softAPIP() : WiFi.localIP();
    log_i(TAG_WS, "HTTP server started → http://%s/", serverIP.toString().c_str());
}

void WebServer::stop() {
    if (!_ws_started) return;
    _ws.end();
    _ws_started = false;
    log_i(TAG_WS, "HTTP server stopped");
}
