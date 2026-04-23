#include "app_config.h"
#include "settings.h"
#include "nvs_table.h"

void loadAppConfig(AppConfig &cfg) {
    Settings s(NVS_NAMESPACE_WEATHER, /*read_write=*/false);

    cfg.wifiSsid      = s.GetString(NVS_KEY_WIFI_SSID,       DEFAULT_WIFI_SSID);
    cfg.wifiPassword  = s.GetString(NVS_KEY_WIFI_PASSWORD,   DEFAULT_WIFI_PASSWORD);
    cfg.lat           = s.GetString(NVS_KEY_LAT,             DEFAULT_LAT);
    cfg.lon           = s.GetString(NVS_KEY_LON,             DEFAULT_LON);
    cfg.city          = s.GetString(NVS_KEY_CITY,            DEFAULT_CITY);
    cfg.timezone      = s.GetString(NVS_KEY_TIMEZONE,        DEFAULT_TIMEZONE);
    cfg.timeFormat    = s.GetString(NVS_KEY_TIME_FORMAT,     DEFAULT_TIME_FORMAT);
    cfg.dateFormat    = s.GetString(NVS_KEY_DATE_FORMAT,     DEFAULT_DATE_FORMAT);
    cfg.sleepDuration = s.GetI32   (NVS_KEY_SLEEP_DURATION,  DEFAULT_SLEEP_DURATION);
    cfg.bedTime       = s.GetI32   (NVS_KEY_BED_TIME,        DEFAULT_BED_TIME);
    cfg.wakeTime      = s.GetI32   (NVS_KEY_WAKE_TIME,       DEFAULT_WAKE_TIME);
    cfg.unitsTemp     = s.GetString(NVS_KEY_UNITS_TEMP,      DEFAULT_UNITS_TEMP);
    cfg.unitsSpeed    = s.GetString(NVS_KEY_UNITS_SPEED,     DEFAULT_UNITS_SPEED);
    cfg.unitsPres     = s.GetString(NVS_KEY_UNITS_PRES,      DEFAULT_UNITS_PRES);
    cfg.unitsDist     = s.GetString(NVS_KEY_UNITS_DIST,      DEFAULT_UNITS_DIST);
    cfg.unitsPrecip   = s.GetString(NVS_KEY_UNITS_PRECIP,    DEFAULT_UNITS_PRECIP);
    cfg.language      = s.GetString(NVS_KEY_LANGUAGE,        DEFAULT_LANGUAGE);
}

void saveAppConfig(const AppConfig &cfg) {
    Settings s(NVS_NAMESPACE_WEATHER, /*read_write=*/true);

    s.SetString(NVS_KEY_WIFI_SSID,      cfg.wifiSsid);
    s.SetString(NVS_KEY_WIFI_PASSWORD,  cfg.wifiPassword);
    s.SetString(NVS_KEY_LAT,            cfg.lat);
    s.SetString(NVS_KEY_LON,            cfg.lon);
    s.SetString(NVS_KEY_CITY,           cfg.city);
    s.SetString(NVS_KEY_TIMEZONE,       cfg.timezone);
    s.SetString(NVS_KEY_TIME_FORMAT,    cfg.timeFormat);
    s.SetString(NVS_KEY_DATE_FORMAT,    cfg.dateFormat);
    s.SetI32   (NVS_KEY_SLEEP_DURATION, cfg.sleepDuration);
    s.SetI32   (NVS_KEY_BED_TIME,       cfg.bedTime);
    s.SetI32   (NVS_KEY_WAKE_TIME,      cfg.wakeTime);
    s.SetString(NVS_KEY_UNITS_TEMP,     cfg.unitsTemp);
    s.SetString(NVS_KEY_UNITS_SPEED,    cfg.unitsSpeed);
    s.SetString(NVS_KEY_UNITS_PRES,     cfg.unitsPres);
    s.SetString(NVS_KEY_UNITS_DIST,     cfg.unitsDist);
    s.SetString(NVS_KEY_UNITS_PRECIP,   cfg.unitsPrecip);
    s.SetString(NVS_KEY_LANGUAGE,       cfg.language);
    s.Commit();
}
