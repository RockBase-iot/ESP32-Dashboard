#include "dashboardApp.h"
#include "bsp/IBoard.h"
#include "ui/ui_layout.h"
#include "app/config/app_config.h"
#include "app/weather/weather.h"
#include "app/wifi/wifi_manager.h"
#include "app/web/web_server.h"
#include "app/locale/locale_mgr.h"
#include "utils/logger.h"
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_mac.h>

static const char *TAG = "DashboardApp";

// ── RTC-retained state (survives deep sleep) ───────────────────────────────
RTC_DATA_ATTR uint8_t DashboardApp::_failCount = 0;
RTC_DATA_ATTR bool    DashboardApp::_coldBoot  = true;
RTC_DATA_ATTR bool    DashboardApp::_stayAwake = false;
RTC_DATA_ATTR bool    DashboardApp::_apMode    = false;

// ═════════════════════════════════════════════════════════════════════════════
// Phase 0 — Wakeup detection
// ═════════════════════════════════════════════════════════════════════════════
// Called before board.init() on EXT0 wakeup for accurate button timing.
// External pull-up: normal = HIGH, pressed = LOW.
// Held < 2 s → SHORT_PRESS (stay-awake); held ≥ 2 s → LONG_PRESS (AP mode).
void DashboardApp::_detectWakeup() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    bool isTimerWake  = (cause == ESP_SLEEP_WAKEUP_TIMER);
    bool isButtonWake = (cause == ESP_SLEEP_WAKEUP_EXT0);

    if (isButtonWake) {
        uint8_t pin = getBoard().bootButtonPin();
        pinMode(pin, INPUT); // external pull-up; no internal pull needed
        unsigned long start = millis();
        bool longPress = false;
        while (digitalRead(pin) == LOW) {
            if (millis() - start >= kLongPressMs) { longPress = true; break; }
            delay(10);
        }
        if (longPress) _apMode    = true;
        else           _stayAwake = true;
    }

    if (!isTimerWake) {
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
                              const AppConfig &cfg, String &outLocalIP) {
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

    outLocalIP = WiFi.localIP().toString();
    log_i(TAG, "Local IP: %s", outLocalIP.c_str());

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

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2b-v — Web portal
// ═════════════════════════════════════════════════════════════════════════════
void DashboardApp::_runWebPortal(WifiManager &wifi, const AppConfig &cfg) {
    if (!_stayAwake) return;

    log_i(TAG, "Stay-awake mode: web portal for %lu min", kStayAwakeMs / 60000UL);
    wifi.connect(cfg.wifiSsid, cfg.wifiPassword);
    {
        WebServer webServer;
        webServer.start();
        log_i(TAG, "Web portal running — open http://%s/ in a browser",
              WiFi.localIP().toString().c_str());
        uint32_t start = millis();
        while (millis() - start < kStayAwakeMs) {
            log_w(TAG, "Stay-awake... %lu s remaining",
                  (kStayAwakeMs - (millis() - start)) / 1000UL);
            delay(1000);
        }
    }
    log_i(TAG, "Stay-awake timeout — going to sleep");
    _stayAwake = false;
    wifi.disconnect();
}

// ═════════════════════════════════════════════════════════════════════════════
// Utility — error page
// ═════════════════════════════════════════════════════════════════════════════
void DashboardApp::_showErrorPage(IBoard &board, const char *title, const char *msg) {
    PageError errorPage;
    errorPage.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                     board.colorAccent(), board.hasAccentColor());
    errorPage.setMessage(title, msg);
    board.epd().firstPage();
    do { errorPage.draw(); } while (board.epd().nextPage());
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

    AppConfig cfg;
    loadAppConfig(cfg);
    log_i(TAG, "Config: ssid=%s  lat=%s  lon=%s  utcOffset=%+d  lang=%s  sleep=%dmin",
          cfg.wifiSsid.c_str(), cfg.lat.c_str(), cfg.lon.c_str(),
          cfg.utcOffset, cfg.language.c_str(), cfg.sleepDuration);

    _initHardware(board, _coldBoot);

    if (_apMode)   { _enterApMode(board); /* never returns */ }
    if (_coldBoot) {
        char splashBuf[64];
        if (_stayAwake) {
            snprintf(splashBuf, sizeof(splashBuf), "Waking up...\nOnline for %lu min",
                     kStayAwakeMs / 60000UL);
        } else {
            snprintf(splashBuf, sizeof(splashBuf), "Connecting to\n%s...",
                     cfg.wifiSsid.c_str());
        }
        _showLoadingPage(board, splashBuf);
    }
    _coldBoot = false;

    WifiManager wifi;
    if (!_connectAndSync(board, wifi, cfg)) {
        log_i(TAG, "Sleeping for %d minutes", cfg.sleepDuration);
        board.epd().hibernate();
        board.deepSleep(static_cast<uint64_t>(cfg.sleepDuration) * 60ULL * 1000000ULL);
        return;
    }

    WeatherClass weather;
    String localIP;
    bool dataOk = _fetchData(board, weather, cfg, localIP);
    wifi.disconnect();
    log_i(TAG, "WiFi disconnected");

    if (!dataOk) {
        log_i(TAG, "Sleeping for %d minutes", cfg.sleepDuration);
        board.epd().hibernate();
        board.deepSleep(static_cast<uint64_t>(cfg.sleepDuration) * 60ULL * 1000000ULL);
        return;
    }

    _renderWeather(board, weather, cfg, localIP);
    _runWebPortal(wifi, cfg);
    log_i(TAG, "Sleeping for %d minutes", cfg.sleepDuration);
    board.epd().hibernate();
    board.deepSleep(static_cast<uint64_t>(cfg.sleepDuration) * 60ULL * 1000000ULL);
}

