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

// ─── Default values (used when NVS key is absent) ─────────────────────────
// Open-Meteo is fully free and requires no API key.
#define DEFAULT_WIFI_SSID        "NMTech-2.4G"
#define DEFAULT_WIFI_PASSWORD    "NMMiner2048"
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
