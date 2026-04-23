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

// ── Debug switch ───────────────────────────────────────────────────────────
// Set to 1 to disable deep sleep (useful during development).
// Set to 0 to re-enable normal deep sleep behaviour.
#define DISABLE_DEEP_SLEEP 0

// Maximum consecutive fetch failures before showing an error page.
#define MAX_FETCH_RETRIES 3

// ── RTC-retained state (survives deep sleep) ───────────────────────────────
RTC_DATA_ATTR static uint8_t g_failCount  = 0;   // consecutive fetch failures
RTC_DATA_ATTR static bool    g_coldBoot   = true; // true only on first power-on
RTC_DATA_ATTR static bool    g_stayAwake  = false; // boot short-press: stay awake 5 min with web portal
RTC_DATA_ATTR static bool    g_apMode     = false; // boot long-press: enter AP config mode

// ── Button press classification ───────────────────────────────────────────
enum class ButtonPress { SHORT_PRESS, LONG_PRESS };

// Call immediately on EXT0 wakeup, before board.init(), for accurate timing.
// External pull-up assumed: normal = HIGH, pressed = LOW.
// Holds up to 2 s: releases before 2 s → SHORT_PRESS, still held → LONG_PRESS.
static ButtonPress detectBootPress(uint8_t pin) {
    pinMode(pin, INPUT); // external pull-up; no internal pull needed
    unsigned long start = millis();
    while (digitalRead(pin) == LOW) {
        if (millis() - start >= 2000UL) return ButtonPress::LONG_PRESS;
        delay(10);
    }
    return ButtonPress::SHORT_PRESS;
}

static void doDeepSleep(IBoard &board, int minutes) {
#if DISABLE_DEEP_SLEEP
    log_i(TAG, "Deep sleep disabled (DISABLE_DEEP_SLEEP=1), halting loop");
    board.epd().hibernate();
    while (true) { 
        delay(1000); 
        log_w(TAG, "Deep sleep disabled... still here after %lu seconds",
              static_cast<unsigned long>(millis() / 1000UL));
    }
#else
    log_i(TAG, "Sleeping for %d minutes", minutes);
    board.epd().hibernate();
    board.deepSleep(static_cast<uint64_t>(minutes) * 60ULL * 1000000ULL);
#endif
}

void DashboardApp::run() {
    // ── 0. Detect wakeup cause + button press (before board.init for accurate timing) ──
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    bool isTimerWake  = (cause == ESP_SLEEP_WAKEUP_TIMER);
    bool isButtonWake = (cause == ESP_SLEEP_WAKEUP_EXT0);

    if (isButtonWake) {
        // Read the boot button pin from the board before full init.
        // getBoard() is safe to call early; it only constructs the singleton.
        ButtonPress press = detectBootPress(getBoard().bootButtonPin());
        if (press == ButtonPress::LONG_PRESS)  g_apMode    = true;
        if (press == ButtonPress::SHORT_PRESS) g_stayAwake = true;
    }

    if (!isTimerWake) {
        g_coldBoot  = true;
        g_failCount = 0;
    }

    // ── 1. Board-level initialization ─────────────────────────────────────
    IBoard &board = getBoard();
    board.init();
    delay(2000); // allow serial port to settle

    log_i(TAG, "=== ESP32-Dashboard starting (cause=%d, cold=%d, fails=%d) ===",
          (int)cause, g_coldBoot, g_failCount);

    // ── 2. Load configuration ──────────────────────────────────────────────
    AppConfig cfg;
    loadAppConfig(cfg);
    log_i(TAG, "Config: ssid=%s  lat=%s  lon=%s  tz=%s  lang=%s  sleep=%dmin",
          cfg.wifiSsid.c_str(), cfg.lat.c_str(), cfg.lon.c_str(),
          cfg.timezone.c_str(), cfg.language.c_str(), cfg.sleepDuration);

    // ── 3. Initialize board hardware ──────────────────────────────────────
    log_i(TAG, "Initializing board hardware");
    // Pass initialPowerOn=true only on cold boot; timer wake skips EPD power-on
    // sequence to avoid a blank flash and speed up the update cycle.
    board.epd().init(/*initialPowerOn=*/g_coldBoot);
    log_i(TAG, "EPD init done (%dx%d)", board.dispWidth(), board.dispHeight());

    if (board.getTempSensor()) {
        bool sensorOk = board.getTempSensor()->begin();
        log_i(TAG, "Sensor %s init: %s", board.getTempSensor()->typeName(),
              sensorOk ? "OK" : "FAILED");
    }

    // ── 3a. AP config mode (long press on boot button) ────────────────────
    if (g_apMode) {
        // Build SSID from chip MAC (esp_read_mac works before WiFi init).
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char apSsid[32];
        snprintf(apSsid, sizeof(apSsid), "esp_dashboard_%02x%02x%02x",
                 mac[3], mac[4], mac[5]);

        log_i(TAG, "Entering AP config mode: SSID=%s", apSsid);
        WifiManager apWifi;
        apWifi.startAP(apSsid);

        // Show AP info on EPD after AP starts so we can display the real IP.
        char apStatus[64];
        snprintf(apStatus, sizeof(apStatus), "AP: %s  %s",
                 apSsid, WiFi.softAPIP().toString().c_str());
        PageLoading apPage;
        apPage.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                      board.colorAccent(), board.hasAccentColor());
        apPage.setStatus(apStatus);
        board.epd().firstPage();
        do { apPage.draw(); } while (board.epd().nextPage());
        WebServer apWebServer;
        apWebServer.start();

        // Block for up to 10 minutes, then restart to resume normal operation.
        unsigned long apStart = millis();
        while (millis() - apStart < 10UL * 60UL * 1000UL) { delay(200); }
        log_i(TAG, "AP mode timeout — restarting");
        g_apMode = false;
        ESP.restart();
        return;
    }

    // ── 4. Show loading page (cold boot only) ─────────────────────────────
    if (g_coldBoot) {
        log_i(TAG, "Drawing loading page");
        PageLoading loading;
        loading.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                       board.colorAccent(), board.hasAccentColor());
        loading.setStatus("Connecting...");
        board.epd().firstPage();
        do { loading.draw(); } while (board.epd().nextPage());
    }

    g_coldBoot = false; // subsequent wakes are timer wakes

    // ── 5. Connect WiFi + sync time ────────────────────────────────────────
    log_i(TAG, "Connecting WiFi: %s", cfg.wifiSsid.c_str());
    WifiManager wifi;
    if (!wifi.connect(cfg.wifiSsid, cfg.wifiPassword)) {
        log_e(TAG, "WiFi connection failed");
        g_failCount++;
        log_w(TAG, "Fetch fail %d/%d — will retry on next wake",
              g_failCount, MAX_FETCH_RETRIES);
        if (g_failCount >= MAX_FETCH_RETRIES) {
            g_failCount = 0;
            PageError errorPage;
            errorPage.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                             board.colorAccent(), board.hasAccentColor());
            errorPage.setMessage("WiFi Error", "Could not connect to network.");
            board.epd().firstPage();
            do { errorPage.draw(); } while (board.epd().nextPage());
        }
        doDeepSleep(board, cfg.sleepDuration);
        return;
    }
    log_i(TAG, "WiFi connected, syncing time (tz=%s)", cfg.timezone.c_str());
    wifi.syncTime(cfg.timezone.c_str());

    // ── 6. Fetch weather and AQI ───────────────────────────────────────────
    double lat = cfg.lat.toDouble();
    double lon = cfg.lon.toDouble();
    log_i(TAG, "Fetching weather: lat=%.4f lon=%.4f", lat, lon);
    WeatherClass weather;
    bool weatherOk = weather.fetchWeather(lat, lon);
    log_i(TAG, "Weather fetch: %s", weatherOk ? "OK" : "FAILED");

    bool aqiOk = false;
    if (weatherOk) {
        log_i(TAG, "Fetching AQI");
        aqiOk = weather.fetchAirQuality(lat, lon);
        log_i(TAG, "AQI fetch: %s", aqiOk ? "OK" : "FAILED");
    }
    // Store local IP before disconnect so the render can display it.
    String localIP = WiFi.localIP().toString();
    log_i(TAG, "Local IP: %s", localIP.c_str());

    wifi.disconnect();
    log_i(TAG, "WiFi disconnected");

    if (!weatherOk) {
        g_failCount++;
        log_w(TAG, "Fetch fail %d/%d — will retry on next wake",
              g_failCount, MAX_FETCH_RETRIES);
        if (g_failCount >= MAX_FETCH_RETRIES) {
            g_failCount = 0;
            PageError errorPage;
            errorPage.create(board.gfx(), board.dispWidth(), board.dispHeight(),
                             board.colorAccent(), board.hasAccentColor());
            errorPage.setMessage("Weather Error", "Failed to fetch weather data.");
            board.epd().firstPage();
            do { errorPage.draw(); } while (board.epd().nextPage());
        }
        doDeepSleep(board, cfg.sleepDuration);
        return;
    }

    // Fetch succeeded — reset failure counter.
    g_failCount = 0;

    // ── 7. Render weather page ─────────────────────────────────────────────
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

    // ── 8. Web portal / stay-awake portal ─────────────────────────────────
#if DISABLE_DEEP_SLEEP
    wifi.connect(cfg.wifiSsid, cfg.wifiPassword);
    {
        WebServer webServer;
        webServer.start();
        log_i(TAG, "Web portal running — open http://%s/ in a browser",
              WiFi.localIP().toString().c_str());
    }
#else
    if (g_stayAwake) {
        uint32_t stayAwakeDuration = 1UL * 60UL * 1000UL; // 5 minutes
        log_i(TAG, "Stay-awake mode: reconnecting WiFi for %lu min with web portal", stayAwakeDuration / (60UL * 1000UL));
        wifi.connect(cfg.wifiSsid, cfg.wifiPassword);
        {
            WebServer webServer;
            webServer.start();
            log_i(TAG, "Web portal running — open http://%s/ in a browser", WiFi.localIP().toString().c_str());
            uint32_t wakeStart = millis();
            while (millis() - wakeStart < stayAwakeDuration) { 
                log_w(TAG, "Stay-awake mode... %lu seconds remaining", (stayAwakeDuration - (millis() - wakeStart)) / 1000UL);
                delay(1000); 
            }
        }
        log_i(TAG, "Stay-awake timeout — going to sleep");
        g_stayAwake = false;
        wifi.disconnect();
    }
#endif
    // ── 9. Deep sleep ──────────────────────────────────────────────────────
    doDeepSleep(board, cfg.sleepDuration);
}
