#pragma once

#include <stdint.h>

#include "app/input/button_controller.h"
#include "app/page/page_catalog.h"
#include "app/time/focus_clock_model.h"

// Forward declarations for private method parameters.
class  IBoard;
class  WifiManager;
class  WeatherClass;
class  PageManager;
class  DisplayPageState;
struct AppConfig;
struct CalendarPageSnapshot;
class  String;
namespace calm_grid { struct ChromeContext; }

// DashboardApp — top-level application controller.
//
// run() orchestrates the complete wake cycle:
//   0. Detect wakeup cause and classify boot-button press
//   1. Initialize board hardware and load config
//   2a. Long press  → AP config mode (never returns, restarts after timeout)
//   2b. Normal flow → loading → connect → fetch → render → sleep
class DashboardApp {
public:
    // Entry point — never returns (ends with deep sleep or restart).
    static void run();

private:
    // ── Tuning constants ───────────────────────────────────────────────────
    static constexpr uint32_t kSerialSettleMs  =  2000;            // ms after board.init()
    static constexpr uint32_t kLongPressMs     =  2000;            // long-press threshold
    static constexpr uint32_t kApTimeoutMs     = 360 * 1000;       // AP mode auto-exit (360 s)
    static constexpr uint32_t kInteractiveIdleSec = 30;            // deep-sleep entry threshold
    static constexpr int      kMaxFetchRetries =  3;               // failures before error page

    // ── RTC-retained state (survives deep sleep) ───────────────────────────
    static uint8_t _failCount;  // consecutive fetch failures
    static bool    _coldBoot;   // true only on first power-on
    static bool    _apMode;     // long press:  AP config mode
    static bool    _restorePersistedPage; // true after deep-sleep wake
    static ButtonAction _lastButtonAction;
    static FocusClockRuntimeState _focusClockState;

    // ── Phase helpers (called in order from run()) ─────────────────────────

    // 0. Read wakeup cause; classify boot-button press.
    //    Updates _apMode / _coldBoot / _failCount.
    static void _detectWakeup();

    // 1. Init EPD and optional on-board sensors.
    static void _initHardware(IBoard &board, bool coldBoot);

    // 2a. SoftAP config portal — starts AP + web server, restarts after timeout.
    static void _enterApMode(IBoard &board);

    // 2b-i. Draw a splash on the EPD (cold boot / button wake).
    static void _showLoadingPage(IBoard &board, const char *status = "Connecting...");

    // 2b-ii. Connect WiFi + sync SNTP. Returns false on failure.
    static bool _connectAndSync(IBoard &board, WifiManager &wifi, const AppConfig &cfg);

    // 2b-iii. Fetch weather + AQI; capture local IP before disconnect.
    //         Returns false on failure.
    static bool _fetchData(IBoard &board, WeatherClass &weather,
                          const AppConfig &cfg, String &outLocalIP,
                          bool fetchWeather);

    // 2b-iv. Render the weather page on the EPD.
    static void _renderWeather(IBoard &board, WeatherClass &weather,
                               const AppConfig &cfg, const String &localIP);

    // 2b-iv-b. Render a configured dashboard page.
    static void _renderDashboardPage(IBoard &board, PageManager &pageManager, PageId page,
                                     int64_t nowUtc, WeatherClass &weather,
                                     const CalendarPageSnapshot &calendarSnapshot,
                                     const AppConfig &cfg, const String &localIP,
                                     const calm_grid::ChromeContext &chrome);
    static void _renderDisplayPage(IBoard &board, PageManager &pageManager,
                                   DisplayPageState page, int64_t nowUtc,
                                   WeatherClass &weather,
                                   const CalendarPageSnapshot &calendarSnapshot,
                                   const AppConfig &cfg,
                                   const String &localIP,
                                   const calm_grid::ChromeContext &chrome);

    // 2b-vi. Awake interactive window; BOOT=next, USER=previous.
    static DisplayPageState _runInteractiveWindow(IBoard &board, PageManager &pageManager,
                                                  DisplayPageState currentPage,
                                                  WeatherClass &weather,
                                                  const CalendarPageSnapshot &calendarSnapshot,
                                                  const AppConfig &cfg,
                                                  const String &localIP);

    // 2b-vii. Power-on config window (PortalSec > 0): web portal + buttons.
    static DisplayPageState _runConfigWindow(IBoard &board, PageManager &pageManager,
                                             DisplayPageState currentPage,
                                             WeatherClass &weather,
                                             const CalendarPageSnapshot &calendarSnapshot,
                                             const AppConfig &cfg,
                                             const String &localIP);
    static DisplayPageState _runFocusClockSession(IBoard &board, PageManager &pageManager,
                                                  DisplayPageState currentPage,
                                                  WeatherClass &weather,
                                                  const CalendarPageSnapshot &calendarSnapshot,
                                                  const AppConfig &cfg,
                                                  const String &localIP);

    // Utility: draw a full-screen error message on the EPD.
    static void _showErrorPage(IBoard &board, const char *title, const char *msg);

    // Utility: persist current page, shut radios/peripherals down, then enter deep sleep.
    static void _enterScheduledSleep(IBoard &board, uint64_t deepSleepUs,
                                     DisplayPageState currentPage);
};
