#include "app_config.h"
#include "settings.h"
#include "nvs_table.h"
#include "legacy_config_migration.h"
#include "app/page/page_manager.h"
#include "app/web/web_config_validation.h"
#include "utils/logger.h"

namespace {
constexpr const char *TAG_CONFIG = "AppConfig";

class SettingsLegacyConfigStore final : public ILegacyConfigStore {
public:
    explicit SettingsLegacyConfigStore(Settings &settings) : _settings(settings) {}

    LegacyKeyEraseResult eraseKey(const char *key) override {
        switch (_settings.EraseKey(key)) {
            case SettingsEraseResult::Removed:
                return LegacyKeyEraseResult::Removed;
            case SettingsEraseResult::NotFound:
                return LegacyKeyEraseResult::NotFound;
            case SettingsEraseResult::Error:
                return LegacyKeyEraseResult::Error;
        }
        return LegacyKeyEraseResult::Error;
    }

    bool commit() override {
        return _settings.Commit();
    }

private:
    Settings &_settings;
};

void runLegacyConfigMigrations() {
    Settings settings(NVS_NAMESPACE_WEATHER, /*read_write=*/true);
    SettingsLegacyConfigStore store(settings);
    const LegacyConfigMigrationResult result = migrateLegacyAccessCredential(store);
    if (result == LegacyConfigMigrationResult::Removed) {
        log_i(TAG_CONFIG, "Removed legacy access credential");
    } else if (result == LegacyConfigMigrationResult::Error) {
        log_w(TAG_CONFIG, "Could not remove legacy access credential");
    }
}

uint16_t clampFocusMinutes(int32_t minutes) {
    if (minutes < 1) {
        return 1;
    }
    if (minutes > 180) {
        return 180;
    }
    return static_cast<uint16_t>(minutes);
}

uint16_t clampFocusBreakMinutes(int32_t minutes) {
    if (minutes < 1) {
        return 1;
    }
    if (minutes > 60) {
        return 60;
    }
    return static_cast<uint16_t>(minutes);
}

uint8_t clampFocusSessionCount(int32_t count) {
    if (count < 1) {
        return 1;
    }
    if (count > 12) {
        return 12;
    }
    return static_cast<uint8_t>(count);
}

String encodePageOrder(const uint8_t *order, size_t count) {
    String out;
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) out += ",";
        out += String(static_cast<int>(order[i]));
    }
    return out;
}

void copyDefaultPageOrder(AppConfig &cfg, const PageSettings &defaults) {
    cfg.pageOrderCount = defaults.orderCount;
    if (cfg.pageOrderCount > APP_CONFIG_PAGE_ORDER_MAX) {
        cfg.pageOrderCount = APP_CONFIG_PAGE_ORDER_MAX;
    }
    for (size_t i = 0; i < cfg.pageOrderCount; ++i) {
        cfg.pageOrder[i] = static_cast<uint8_t>(defaults.order[i]);
    }
}

void copyPageSettingsToConfig(AppConfig &cfg, const PageSettings &settings) {
    cfg.configVersion = settings.configVersion;
    cfg.pageEnabledMask = settings.enabledMask;
    cfg.pageAutoRotateMask = settings.autoRotateMask;
    cfg.pageTemplateId = static_cast<uint8_t>(settings.templateId);
    cfg.rotationIntervalMinutes = settings.rotationIntervalMinutes;
    cfg.timeZoneId = String(settings.timeZoneId.c_str());
    copyDefaultPageOrder(cfg, settings);
}

bool parseUint8Token(const String &token, uint8_t &out) {
    if (token.length() == 0) {
        return false;
    }
    int value = 0;
    for (int i = 0; i < token.length(); ++i) {
        const char c = token.charAt(i);
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
        if (value >= static_cast<int>(APP_CONFIG_PAGE_ORDER_MAX)) {
            return false;
        }
    }
    out = static_cast<uint8_t>(value);
    return true;
}

bool parsePageOrder(const String &csv, AppConfig &cfg) {
    cfg.pageOrderCount = 0;
    if (csv.length() == 0) {
        return false;
    }
    bool seen[APP_CONFIG_PAGE_ORDER_MAX] = {};
    int start = 0;
    while (start <= csv.length()) {
        if (cfg.pageOrderCount >= APP_CONFIG_PAGE_ORDER_MAX) {
            return false;
        }
        int comma = csv.indexOf(',', start);
        if (comma < 0) comma = csv.length();
        const String token = csv.substring(start, comma);
        uint8_t value = 0;
        if (!parseUint8Token(token, value) || seen[value]) {
            return false;
        }
        seen[value] = true;
        cfg.pageOrder[cfg.pageOrderCount++] = value;
        start = comma + 1;
        if (comma == csv.length()) break;
    }
    return cfg.pageOrderCount > 0;
}
}  // namespace

void loadAppConfig(AppConfig &cfg) {
    runLegacyConfigMigrations();
    Settings s(NVS_NAMESPACE_WEATHER, /*read_write=*/false);
    const PageSettings pageDefaults = defaultPageSettings();

    cfg.wifiSsid      = s.GetString(NVS_KEY_WIFI_SSID,       DEFAULT_WIFI_SSID);
    cfg.wifiPassword  = s.GetString(NVS_KEY_WIFI_PASSWORD,   DEFAULT_WIFI_PASSWORD);
    cfg.lat           = s.GetString(NVS_KEY_LAT,             DEFAULT_LAT);
    cfg.lon           = s.GetString(NVS_KEY_LON,             DEFAULT_LON);
    cfg.city          = s.GetString(NVS_KEY_CITY,            DEFAULT_CITY);
    cfg.utcOffset     = s.GetI32   (NVS_KEY_TIMEZONE,        DEFAULT_UTC_OFFSET);
    cfg.timeFormat    = s.GetString(NVS_KEY_TIME_FORMAT,     DEFAULT_TIME_FORMAT);
    cfg.dateFormat    = s.GetString(NVS_KEY_DATE_FORMAT,     DEFAULT_DATE_FORMAT);
    cfg.sleepDuration = s.GetI32   (NVS_KEY_SLEEP_DURATION,  DEFAULT_SLEEP_DURATION);
    cfg.bedTime       = s.GetI32   (NVS_KEY_BED_TIME,        DEFAULT_BED_TIME);
    cfg.wakeTime      = s.GetI32   (NVS_KEY_WAKE_TIME,       DEFAULT_WAKE_TIME);
    cfg.portalWindowSec = normalizePortalWindowSec(
        s.GetI32(NVS_KEY_PORTAL_WINDOW, DEFAULT_PORTAL_WINDOW_SEC));
    cfg.unitsTemp     = s.GetString(NVS_KEY_UNITS_TEMP,      DEFAULT_UNITS_TEMP);
    cfg.unitsSpeed    = s.GetString(NVS_KEY_UNITS_SPEED,     DEFAULT_UNITS_SPEED);
    cfg.unitsPres     = s.GetString(NVS_KEY_UNITS_PRES,      DEFAULT_UNITS_PRES);
    cfg.unitsDist     = s.GetString(NVS_KEY_UNITS_DIST,      DEFAULT_UNITS_DIST);
    cfg.unitsPrecip   = s.GetString(NVS_KEY_UNITS_PRECIP,    DEFAULT_UNITS_PRECIP);
    cfg.language      = s.GetString(NVS_KEY_LANGUAGE,        DEFAULT_LANGUAGE);
    cfg.configVersion = static_cast<uint32_t>(s.GetI32(NVS_KEY_CONFIG_VERSION, DEFAULT_CONFIG_VERSION));
    cfg.pageEnabledMask = static_cast<uint32_t>(
        s.GetI32(NVS_KEY_PAGE_ENABLED, static_cast<int32_t>(pageDefaults.enabledMask)));
    cfg.pageAutoRotateMask = static_cast<uint32_t>(
        s.GetI32(NVS_KEY_PAGE_AUTO, static_cast<int32_t>(pageDefaults.autoRotateMask)));
    cfg.pageTemplateId = static_cast<uint8_t>(s.GetI32(NVS_KEY_PAGE_TEMPLATE, DEFAULT_PAGE_TEMPLATE));
    cfg.rotationIntervalMinutes = static_cast<uint16_t>(
        s.GetI32(NVS_KEY_ROTATE_MINUTES, DEFAULT_ROTATE_MINUTES));
    cfg.timeZoneId = s.GetString(NVS_KEY_TIME_ZONE_ID, "");
    if (cfg.timeZoneId.length() == 0) {
        cfg.timeZoneId = String(timeZoneIdForUtcOffset(cfg.utcOffset).c_str());
    }
    if (!parsePageOrder(s.GetString(NVS_KEY_PAGE_ORDER, ""), cfg)) {
        copyDefaultPageOrder(cfg, pageDefaults);
    }
    cfg.newsFeeds = s.GetString(NVS_KEY_NEWS_FEEDS, DEFAULT_NEWS_FEEDS);
    cfg.stockSymbols = s.GetString(NVS_KEY_STOCK_SYMBOLS, DEFAULT_STOCK_SYMBOLS);
    cfg.portfolioPositions = s.GetString(NVS_KEY_PORTFOLIO_POS, DEFAULT_PORTFOLIO_POS);
    cfg.economicFeeds = s.GetString(NVS_KEY_ECONOMIC_FEEDS, DEFAULT_ECONOMIC_FEEDS);
    cfg.worldClockZones = s.GetString(NVS_KEY_WORLD_ZONES, DEFAULT_WORLD_ZONES);
    cfg.focusLabel = s.GetString(NVS_KEY_FOCUS_LABEL, DEFAULT_FOCUS_LABEL);
    cfg.focusMinutes = clampFocusMinutes(s.GetI32(NVS_KEY_FOCUS_MINUTES,
                                                  DEFAULT_FOCUS_MINUTES));
    cfg.focusBreakMinutes = clampFocusBreakMinutes(
        s.GetI32(NVS_KEY_FOCUS_BREAK_MIN, DEFAULT_FOCUS_BREAK_MIN));
    cfg.focusSessionCount = clampFocusSessionCount(
        s.GetI32(NVS_KEY_FOCUS_SESSIONS, DEFAULT_FOCUS_SESSIONS));
    cfg.indoorSensorEnabled = s.GetBool(NVS_KEY_INDOOR_ENABLED, DEFAULT_INDOOR_ENABLED);
    cfg.indoorRoom = s.GetString(NVS_KEY_INDOOR_ROOM, DEFAULT_INDOOR_ROOM);

    PageSettings rawPageSettings = pageDefaults;
    if (cfg.configVersion == kDashboardConfigVersion) {
        rawPageSettings.configVersion = cfg.configVersion;
        rawPageSettings.enabledMask = cfg.pageEnabledMask;
        rawPageSettings.autoRotateMask = cfg.pageAutoRotateMask;
        rawPageSettings.orderCount = cfg.pageOrderCount;
        for (size_t i = 0; i < rawPageSettings.orderCount && i < rawPageSettings.order.size(); ++i) {
            rawPageSettings.order[i] = static_cast<PageId>(cfg.pageOrder[i]);
        }
        rawPageSettings.templateId = static_cast<PageTemplateId>(cfg.pageTemplateId);
        rawPageSettings.rotationIntervalMinutes = cfg.rotationIntervalMinutes;
    }
    rawPageSettings.timeZoneId = cfg.timeZoneId.c_str();
    copyPageSettingsToConfig(cfg, sanitizePageSettings(rawPageSettings));
}

void saveAppConfig(const AppConfig &cfg) {
    Settings s(NVS_NAMESPACE_WEATHER, /*read_write=*/true);

    s.SetString(NVS_KEY_WIFI_SSID,      cfg.wifiSsid);
    s.SetString(NVS_KEY_WIFI_PASSWORD,  cfg.wifiPassword);
    s.SetString(NVS_KEY_LAT,            cfg.lat);
    s.SetString(NVS_KEY_LON,            cfg.lon);
    s.SetString(NVS_KEY_CITY,           cfg.city);
    s.SetI32   (NVS_KEY_TIMEZONE,       cfg.utcOffset);
    s.SetString(NVS_KEY_TIME_FORMAT,    cfg.timeFormat);
    s.SetString(NVS_KEY_DATE_FORMAT,    cfg.dateFormat);
    s.SetI32   (NVS_KEY_SLEEP_DURATION, cfg.sleepDuration);
    s.SetI32   (NVS_KEY_BED_TIME,       cfg.bedTime);
    s.SetI32   (NVS_KEY_WAKE_TIME,      cfg.wakeTime);
    s.SetI32   (NVS_KEY_PORTAL_WINDOW,  static_cast<int32_t>(cfg.portalWindowSec));
    s.SetString(NVS_KEY_UNITS_TEMP,     cfg.unitsTemp);
    s.SetString(NVS_KEY_UNITS_SPEED,    cfg.unitsSpeed);
    s.SetString(NVS_KEY_UNITS_PRES,     cfg.unitsPres);
    s.SetString(NVS_KEY_UNITS_DIST,     cfg.unitsDist);
    s.SetString(NVS_KEY_UNITS_PRECIP,   cfg.unitsPrecip);
    s.SetString(NVS_KEY_LANGUAGE,       cfg.language);
    s.SetI32   (NVS_KEY_CONFIG_VERSION, static_cast<int32_t>(cfg.configVersion));
    s.SetI32   (NVS_KEY_PAGE_ENABLED,   static_cast<int32_t>(cfg.pageEnabledMask));
    s.SetString(NVS_KEY_PAGE_ORDER,     encodePageOrder(cfg.pageOrder, cfg.pageOrderCount));
    s.SetI32   (NVS_KEY_PAGE_AUTO,      static_cast<int32_t>(cfg.pageAutoRotateMask));
    s.SetI32   (NVS_KEY_PAGE_TEMPLATE,  static_cast<int32_t>(cfg.pageTemplateId));
    s.SetI32   (NVS_KEY_ROTATE_MINUTES, static_cast<int32_t>(cfg.rotationIntervalMinutes));
    s.SetString(NVS_KEY_TIME_ZONE_ID,   cfg.timeZoneId);
    s.SetString(NVS_KEY_NEWS_FEEDS,     cfg.newsFeeds);
    s.SetString(NVS_KEY_STOCK_SYMBOLS,  cfg.stockSymbols);
    s.SetString(NVS_KEY_PORTFOLIO_POS,  cfg.portfolioPositions);
    s.SetString(NVS_KEY_ECONOMIC_FEEDS, cfg.economicFeeds);
    s.SetString(NVS_KEY_WORLD_ZONES,    cfg.worldClockZones);
    s.SetString(NVS_KEY_FOCUS_LABEL,    cfg.focusLabel);
    s.SetI32   (NVS_KEY_FOCUS_MINUTES,  clampFocusMinutes(cfg.focusMinutes));
    s.SetI32   (NVS_KEY_FOCUS_BREAK_MIN,clampFocusBreakMinutes(cfg.focusBreakMinutes));
    s.SetI32   (NVS_KEY_FOCUS_SESSIONS, clampFocusSessionCount(cfg.focusSessionCount));
    s.SetBool  (NVS_KEY_INDOOR_ENABLED, cfg.indoorSensorEnabled);
    s.SetString(NVS_KEY_INDOOR_ROOM,    cfg.indoorRoom);
    s.Commit();
}
