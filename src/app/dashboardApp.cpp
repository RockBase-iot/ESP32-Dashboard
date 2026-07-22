#include "dashboardApp.h"
#include "bsp/IBoard.h"
#include "app/input/button_controller.h"
#include "app/page/page_manager.h"
#include "app/page/page_state_store.h"
#include "app/render/render_coordinator.h"
#include "ui/ui_layout.h"
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
#include <esp_attr.h>
#include <esp_sleep.h>
#include <esp_mac.h>
#include <time.h>

static const char *TAG = "DashboardApp";

// ── RTC-retained state (survives deep sleep) ───────────────────────────────
RTC_DATA_ATTR uint8_t DashboardApp::_failCount = 0;
RTC_DATA_ATTR bool    DashboardApp::_coldBoot  = true;
RTC_DATA_ATTR bool    DashboardApp::_apMode    = false;
RTC_DATA_ATTR ButtonAction DashboardApp::_lastButtonAction = ButtonAction::None;

namespace {
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

uint32_t dashboardContentHash(PageId page, const WeatherClass &weather, const String &localIP) {
    uint32_t hash = 2166136261UL;
    hash = fnv1aAdd(hash, static_cast<uint32_t>(page));
    if (page == PageId::WeatherToday) {
        const WeatherData &data = weather.weather();
        const AirQualityData &aqi = weather.airQuality();
        hash = fnv1aAdd(hash, data.valid ? 1 : 0);
        hash = fnv1aAdd(hash, scaledFloatHash(data.current.temperature, 10.0f));
        hash = fnv1aAdd(hash, scaledFloatHash(data.current.apparent_temperature, 10.0f));
        hash = fnv1aAdd(hash, scaledFloatHash(data.current.humidity, 1.0f));
        hash = fnv1aAdd(hash, static_cast<uint32_t>(data.current.weather_code));
        hash = fnv1aAdd(hash, aqi.valid ? 1 : 0);
        hash = fnv1aAdd(hash, static_cast<uint32_t>(aqi.us_aqi));
        hash = fnv1aAddString(hash, localIP);
    }
    return hash == 0 ? 1 : hash;
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

// Block until both buttons are released (3 s cap). Called before arming the
// low-level light-sleep wake so a still-held key cannot re-fire it instantly.
void waitForButtonsReleased(uint8_t bootPin, uint8_t userPin) {
    const uint32_t start = millis();
    while ((isPressedPin(bootPin) || isPressedPin(userPin)) &&
           millis() - start < 3000UL) {
        delay(20);
    }
}

// Actively poll one press through ButtonController until its action resolves
// (all actions resolve on release). The CPU is awake here by definition — the
// user is touching the device. Capped so a key held forever cannot trap the
// interactive window (RecoveryAp is the longest semantic at 6 s).
ButtonAction pollButtonPress(ButtonId id, uint8_t pin) {
    ButtonController buttons;
    const uint32_t start = millis();
    while (millis() - start < 8000UL) {
        const ButtonAction action = buttons.update(id, isPressedPin(pin), millis());
        if (action != ButtonAction::None) {
            return action;
        }
        if (!isPressedPin(pin)) {
            return ButtonAction::None;  // released without a stable edge (bounce)
        }
        delay(10);
    }
    return ButtonAction::None;
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
    bool isTimerWake  = (cause == ESP_SLEEP_WAKEUP_TIMER);
    bool isBootWake = (cause == ESP_SLEEP_WAKEUP_EXT0);

    _lastButtonAction = ButtonAction::None;
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

void DashboardApp::_renderDashboardPage(IBoard &board, PageManager &pageManager, PageId page,
                                        int64_t nowUtc, WeatherClass &weather,
                                        const AppConfig &cfg, const String &localIP) {
    const PageDescriptor *descriptor = findPage(page);
    log_i(TAG, "Rendering dashboard page: %s (%u)",
          descriptor ? descriptor->name : "Unknown", static_cast<unsigned>(page));
    if (page == PageId::WeatherToday) {
        _renderWeather(board, weather, cfg, localIP);
        return;
    }

#if defined(UI_LAYOUT_EPD_400x300)
    GfxSurface surface(board.gfx());
    board.epd().firstPage();
    do {
        switch (page) {
            case PageId::Overview:
                renderOverviewPage(surface, sampleCalendarPageSnapshot());
                break;
            case PageId::TodayAgenda:
                renderTodayAgendaPage(surface, sampleCalendarPageSnapshot());
                break;
            case PageId::WeeklyTimeline:
                renderWeeklyTimelinePage(surface, sampleCalendarPageSnapshot());
                break;
            case PageId::MonthlyOverview:
                renderMonthlyOverviewPage(surface, sampleCalendarPageSnapshot());
                break;
            case PageId::LocalNotes:
                renderLocalNotesPage(surface, sampleCalendarPageSnapshot());
                break;
            case PageId::WeeklyWeather:
                renderWeeklyWeatherPage(surface, sampleWeatherPageSnapshot());
                break;
            case PageId::IndoorClimate:
                renderIndoorClimatePage(surface, sampleWeatherPageSnapshot());
                break;
            case PageId::WorldClock:
                renderWorldClockPage(surface,
                                      worldClockPageSnapshotAt(nowUtc, cfg.timeZoneId.c_str(),
                                                               pageManager.pageNumber(page),
                                                               pageManager.pageCount()));
                break;
            case PageId::FocusClock:
                renderFocusClockPage(surface,
                                     worldClockPageSnapshotAt(nowUtc, cfg.timeZoneId.c_str(),
                                                              pageManager.pageNumber(page),
                                                              pageManager.pageCount()));
                break;
            case PageId::StockInfo:
                renderStockInfoPage(surface, sampleFinancePageSnapshot());
                break;
            case PageId::PortfolioSummary:
                renderPortfolioSummaryPage(surface, sampleFinancePageSnapshot());
                break;
            case PageId::EconomicCalendar:
                renderEconomicCalendarPage(surface, sampleFinancePageSnapshot());
                break;
            case PageId::Headlines:
                renderHeadlinesPage(surface, sampleNewsPageSnapshot());
                break;
            case PageId::TodayInHistory:
                renderTodayInHistoryPage(surface, sampleNewsPageSnapshot());
                break;
            case PageId::ImportantMilestones:
                renderImportantMilestonesPage(surface, sampleCalendarPageSnapshot());
                break;
            case PageId::WeatherToday:
            default:
                break;
        }
    } while (board.epd().nextPage());
    log_i(TAG, "Render complete");
#else
    _renderWeather(board, weather, cfg, localIP);
#endif
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

PageId DashboardApp::_runInteractiveWindow(IBoard &board, PageManager &pageManager,
                                           WeatherClass &weather, const AppConfig &cfg,
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
    const uint32_t budgetMs = kInteractiveIdleSec * 1000UL;
    while (true) {
        const uint32_t elapsedMs = millis() - lastActivityMs;
        if (elapsedMs >= budgetMs) {
            break;  // never arm light sleep with an empty budget
        }

        // Sleep until a button edge or the remaining inactivity budget expires.
        const LightWake wake = board.lightSleepMs(budgetMs - elapsedMs);
        if (interactiveWindowShouldExit(wake, millis() - lastActivityMs, budgetMs)) {
            break;
        }
        if (wake != LightWake::BootButton && wake != LightWake::UserButton) {
            continue;  // spurious wake — re-arm light sleep
        }

        // A button woke the CPU: classify the press (actions resolve on release).
        const bool isBoot  = (wake == LightWake::BootButton);
        const ButtonAction action = pollButtonPress(isBoot ? ButtonId::Boot : ButtonId::User,
                                                    isBoot ? bootPin : userPin);
        waitForButtonsReleased(bootPin, userPin);
        if (action == ButtonAction::None) {
            continue;  // bounce-only wake
        }

        _lastButtonAction = action;
        log_i(TAG, "Interactive button action: %s", buttonActionName(action));

        if (action == ButtonAction::OpenConfig || action == ButtonAction::RecoveryAp) {
            _apMode = true;
            _enterApMode(board);
            return pageManager.current();
        }

        const PageId selectedPage = applyButtonPageAction(pageManager, action);
        if (action == ButtonAction::NextPage || action == ButtonAction::PreviousPage) {
            _renderDashboardPage(board, pageManager, selectedPage,
                                 static_cast<int64_t>(time(nullptr)),
                                 weather, cfg, localIP);
            const uint32_t contentHash = dashboardContentHash(selectedPage, weather, localIP);
            gDashboardRtcPageState = pageManager.snapshotRtcState(static_cast<int64_t>(time(nullptr)),
                                                                 contentHash);
        }
        // Reset the inactivity budget AFTER processing so the render time does
        // not eat into the user's 30 s.
        lastActivityMs = millis();
    }

    return pageManager.current();
}

// ═════════════════════════════════════════════════════════════════════════════
// Phase 2b-vii — Power-on config window (PortalSec > 0)
// ═════════════════════════════════════════════════════════════════════════════
// Keeps WiFi associated (modem sleep is enabled by the caller) and serves the
// web portal while buttons stay active. Button actions and web requests both
// refresh the inactivity budget; a hard cap bounds the total on-time.
PageId DashboardApp::_runConfigWindow(IBoard &board, PageManager &pageManager,
                                      WeatherClass &weather, const AppConfig &cfg,
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

    while (millis() - lastActivityMs < budgetMs &&
           millis() - windowStartMs < hardCapMs) {
        // Active browsing extends the window (wrap-safe comparison).
        const uint32_t webMs = webServer.lastActivityMs();
        if (static_cast<int32_t>(webMs - lastActivityMs) > 0) {
            lastActivityMs = webMs;
        }

        // Edge-based button polling: actions fire once, on release.
        ButtonAction action = ButtonAction::None;
        if (bootPin != 0xFF) {
            action = buttons.update(ButtonId::Boot, isPressedPin(bootPin), millis());
        }
        if (action == ButtonAction::None && userPin != 0xFF) {
            action = buttons.update(ButtonId::User, isPressedPin(userPin), millis());
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
            return pageManager.current();
        }

        const PageId selectedPage = applyButtonPageAction(pageManager, action);
        if (action == ButtonAction::NextPage || action == ButtonAction::PreviousPage) {
            _renderDashboardPage(board, pageManager, selectedPage,
                                 static_cast<int64_t>(time(nullptr)),
                                 weather, cfg, localIP);
            const uint32_t contentHash = dashboardContentHash(selectedPage, weather, localIP);
            gDashboardRtcPageState = pageManager.snapshotRtcState(static_cast<int64_t>(time(nullptr)),
                                                                 contentHash);
            // Reset the budget AFTER rendering so refresh time is not billed
            // to the user's window.
            lastActivityMs = millis();
        }
        delay(20);
    }

    log_i(TAG, "Config window closed");
    webServer.stop();
    return pageManager.current();
}

void DashboardApp::_enterScheduledSleep(IBoard &board, uint64_t deepSleepUs, PageId currentPage) {
    const bool stored = savePersistedCurrentPage(currentPage);
    log_i(TAG, "Entering deep sleep: deepTimer=%llu us currentPage=%u stored=%d",
          static_cast<unsigned long long>(deepSleepUs),
          static_cast<unsigned>(currentPage),
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
    const PageId persistedPage = loadPersistedCurrentPage(settings);
    const PageId restoredPage = pageManager.restoreStoredPage(persistedPage);
    log_i(TAG, "Restored page from NVS: %u", static_cast<unsigned>(restoredPage));

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
    bool dataOk = _fetchData(board, weather, cfg, localIP);

    if (!dataOk) {
        wifi.disconnect();
        log_i(TAG, "WiFi disconnected");
        log_i(TAG, "Sleeping for %d minutes", cfg.sleepDuration);
        _enterScheduledSleep(board,
                             static_cast<uint64_t>(cfg.sleepDuration) * 60ULL * 1000000ULL,
                             pageManager.current());
        return;
    }

    const RtcPageState previousRtcPageState = gDashboardRtcPageState;
    const PageId previousPage = previousRtcPageState.currentPage;
    const PageId selectedPage = pageManager.current();
    const uint32_t contentHash = dashboardContentHash(selectedPage, weather, localIP);
    const int64_t nowUtc = static_cast<int64_t>(time(nullptr));
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
        _renderDashboardPage(board, pageManager, selectedPage, nowUtc, weather, cfg, localIP);
        gDashboardRtcPageState = pageManager.snapshotRtcState(nowUtc, contentHash);
    } else {
        log_i(TAG, "Render skipped: page=%u deferred=%d hash=0x%08lx lastHash=0x%08lx",
              static_cast<unsigned>(selectedPage),
              renderDecision.deferred,
              static_cast<unsigned long>(contentHash),
              static_cast<unsigned long>(previousRtcPageState.lastContentHash));
        gDashboardRtcPageState = pageManager.snapshotRtcState(previousRtcPageState.lastFullRefreshUtc,
                                                             previousRtcPageState.lastContentHash);
    }
    PageId finalPage;
    if (cfg.portalWindowSec > 0) {
        // Online profile: keep WiFi associated (modem sleep) and serve the
        // web portal for the configured window; buttons stay active too.
        WiFi.setSleep(true);
        finalPage = _runConfigWindow(board, pageManager, weather, cfg, localIP);
    } else {
        // Offline profile: radios off, light-sleep button window.
        wifi.disconnect();
        log_i(TAG, "WiFi disconnected");
        finalPage = _runInteractiveWindow(board, pageManager, weather, cfg, localIP);
    }
    log_i(TAG, "Sleeping for %d minutes", cfg.sleepDuration);
    _enterScheduledSleep(board,
                         static_cast<uint64_t>(cfg.sleepDuration) * 60ULL * 1000000ULL,
                         finalPage);
}

