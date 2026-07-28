#pragma once

// ─── NVS namespace ────────────────────────────────────────────────────────
#define NVS_NAMESPACE_WEATHER    "weather_epd"

// ─── NVS key constants ────────────────────────────────────────────────────
#define NVS_KEY_WIFI_SSID        "WifiSSID"
#define NVS_KEY_WIFI_PASSWORD    "WifiPSWD"
#define NVS_KEY_LAT              "Lat"
#define NVS_KEY_LON              "Lon"
#define NVS_KEY_CITY             "City"
#define NVS_KEY_TIMEZONE         "Timezone"
#define NVS_KEY_SLEEP_DURATION   "SleepMin"
#define NVS_KEY_BED_TIME         "BedTime"
#define NVS_KEY_WAKE_TIME        "WakeTime"
#define NVS_KEY_UNITS_TEMP       "UnitsTemp"    // "C" / "F"
#define NVS_KEY_UNITS_SPEED      "UnitsSpeed"   // "ms" / "mph" / "kmh" / "kn"
#define NVS_KEY_UNITS_PRES       "UnitsPres"    // "hPa" / "inHg" / "mmHg"
#define NVS_KEY_UNITS_DIST       "UnitsDist"    // "km" / "mi"
#define NVS_KEY_UNITS_PRECIP     "UnitsPrecip"  // "mm" / "in"
#define NVS_KEY_TIME_FORMAT      "TimeFmt"
#define NVS_KEY_DATE_FORMAT      "DateFmt"
#define NVS_KEY_LANGUAGE         "Language"     // "en_US" / "zh_CN" / ...
#define NVS_KEY_CONFIG_VERSION   "CfgVer"
#define NVS_KEY_PAGE_ENABLED     "PageEn"
#define NVS_KEY_PAGE_ORDER       "PageOrd"
#define NVS_KEY_PAGE_AUTO        "PageAuto"
#define NVS_KEY_PAGE_TEMPLATE    "PageTpl"
#define NVS_KEY_ROTATE_MINUTES   "RotMin"
#define NVS_KEY_TIME_ZONE_ID     "TzId"
#define NVS_KEY_CURRENT_PAGE     "CurPage"
#define NVS_KEY_PORTAL_WINDOW    "PortalSec"
#define NVS_KEY_NEWS_FEEDS       "NewsFeeds"
#define NVS_KEY_STOCK_SYMBOLS    "StockSyms"
#define NVS_KEY_PORTFOLIO_POS    "PortPos"
#define NVS_KEY_ECONOMIC_FEEDS   "EconFeeds"
#define NVS_KEY_WORLD_ZONES      "WorldZones"
#define NVS_KEY_FOCUS_LABEL      "FocusName"
#define NVS_KEY_FOCUS_MINUTES    "FocusMin"
#define NVS_KEY_FOCUS_BREAK_MIN  "FocusBreak"
#define NVS_KEY_FOCUS_SESSIONS   "FocusCycles"
#define NVS_KEY_INDOOR_ENABLED   "IndoorEn"
#define NVS_KEY_INDOOR_ROOM      "IndoorRoom"

// ─── Default values (used when NVS key is absent) ─────────────────────────
// Open-Meteo is fully free and requires no API key.
#define DEFAULT_WIFI_SSID        ""
#define DEFAULT_WIFI_PASSWORD    ""
#define DEFAULT_LAT              "30.6667"
#define DEFAULT_LON              "104.0667"
#define DEFAULT_CITY             "Chengdu, Sichuan, China"
#define DEFAULT_UTC_OFFSET       8        // UTC+8 (China Standard Time)
#define DEFAULT_SLEEP_DURATION   30       // minutes
#define DEFAULT_BED_TIME         0        // hour (24-h)
#define DEFAULT_WAKE_TIME        6        // hour (24-h)
#define DEFAULT_UNITS_TEMP       "C"
#define DEFAULT_UNITS_SPEED      "kmh"
#define DEFAULT_UNITS_PRES       "hPa"
#define DEFAULT_UNITS_DIST       "km"
#define DEFAULT_UNITS_PRECIP     "mm"
#define DEFAULT_TIME_FORMAT      "%H:%M"
#define DEFAULT_DATE_FORMAT      "%a, %B %e"
#define DEFAULT_LANGUAGE         "zh_CN"
#define DEFAULT_CONFIG_VERSION   4
#define DEFAULT_PAGE_TEMPLATE    0
#define DEFAULT_ROTATE_MINUTES   0
#define DEFAULT_PORTAL_WINDOW_SEC 30      // power-on config window; 0 = disabled
#define DEFAULT_NEWS_FEEDS       "https://feeds.bbci.co.uk/news/rss.xml,https://hnrss.org/frontpage,https://www.nasa.gov/rss/dyn/breaking_news.rss"
#define DEFAULT_STOCK_SYMBOLS    "AAPL.US,MSFT.US,BTCUSD"
#define DEFAULT_PORTFOLIO_POS    ""
#define DEFAULT_ECONOMIC_FEEDS   ""
#define DEFAULT_WORLD_ZONES      "Shanghai|Asia/Shanghai,New York|America/New_York,London|Europe/London,Tokyo|Asia/Tokyo"
#define DEFAULT_FOCUS_LABEL      "Focus"
#define DEFAULT_FOCUS_MINUTES    25
#define DEFAULT_FOCUS_BREAK_MIN  5
#define DEFAULT_FOCUS_SESSIONS   4
#define DEFAULT_INDOOR_ENABLED   false
#define DEFAULT_INDOOR_ROOM      "Indoor"
