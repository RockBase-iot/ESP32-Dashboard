#include "wifi_manager.h"

#include <WiFi.h>
#include <esp_sntp.h>
#include "utils/logger.h"

static const char *TAG = "WifiManager";

static const int CONNECT_TIMEOUT_MS  = 15000;
static const int NTP_SERVER_TIMEOUT_MS = 5000; // per-server wait
static const int NTP_MAX_RETRIES       = 2;    // retries per server

// NTP server pool — tried in order, first success wins.
static const char * const NTP_SERVERS[] = {
    // ── Asia ──────────────────────────────────────────────────────────────
    "ntp.aliyun.com",           // Alibaba CN
    "ntp.tencent.com",          // Tencent CN
    "ntp.ntsc.ac.cn",           // National Time Service Center CN
    "cn.pool.ntp.org",          // NTP Pool CN
    "asia.pool.ntp.org",        // NTP Pool Asia
    "ntp.nict.jp",              // Japan (NICT)
    "time.nist.gov",            // NIST (also serves Asia well)

    // ── Europe ────────────────────────────────────────────────────────────
    "europe.pool.ntp.org",      // NTP Pool Europe
    "de.pool.ntp.org",          // Germany
    "fr.pool.ntp.org",          // France
    "uk.pool.ntp.org",          // United Kingdom
    "ntp.se",                   // Sweden (RISE)
    "time.euro.apple.com",      // Apple Europe

    // ── North America ─────────────────────────────────────────────────────
    "north-america.pool.ntp.org", // NTP Pool North America
    "us.pool.ntp.org",          // United States
    "time.windows.com",         // Microsoft
    "time.apple.com",           // Apple

    // ── South America ─────────────────────────────────────────────────────
    "south-america.pool.ntp.org", // NTP Pool South America
    "a.ntp.br",                 // Brazil (NTP.br)

    // ── Africa ────────────────────────────────────────────────────────────
    "africa.pool.ntp.org",      // NTP Pool Africa
    "za.pool.ntp.org",          // South Africa

    // ── Oceania ───────────────────────────────────────────────────────────
    "oceania.pool.ntp.org",     // NTP Pool Oceania
    "au.pool.ntp.org",          // Australia

    // ── Global fallbacks ──────────────────────────────────────────────────
    "pool.ntp.org",             // Global pool
    "time.cloudflare.com",      // Cloudflare (anycast)
    "time.google.com",          // Google (anycast)
};
static const uint8_t NTP_SERVER_CNT = sizeof(NTP_SERVERS) / sizeof(NTP_SERVERS[0]);

bool WifiManager::connect(const String &ssid, const String &password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    log_i(TAG, "Connecting to %s ...", ssid.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > CONNECT_TIMEOUT_MS) {
            log_e(TAG, "Connection timed out");
            return false;
        }
        delay(250);
    }
    log_i(TAG, "Connected, IP=%s RSSI=%d", WiFi.localIP().toString().c_str(),
          WiFi.RSSI());
    return true;
}

void WifiManager::syncTime(int utcOffsetHours) {
    long offsetSec = (long)utcOffsetHours * 3600L;

    for (uint8_t si = 0; si < NTP_SERVER_CNT; si++) {
        const char *server = NTP_SERVERS[si];
        log_i(TAG, "NTP trying: %s", server);

        esp_sntp_stop();
        configTime(offsetSec, 0, server);

        bool synced = false;
        for (uint8_t retry = 0; retry < NTP_MAX_RETRIES && !synced; retry++) {
            unsigned long start = millis();
            struct tm tm_info;
            while (millis() - start < (unsigned long)NTP_SERVER_TIMEOUT_MS) {
                if (getLocalTime(&tm_info) && tm_info.tm_year > 120) {
                    synced = true;
                    break;
                }
                delay(200);
            }
        }

        if (synced) {
            struct tm tm_info;
            getLocalTime(&tm_info);
            log_i(TAG, "NTP synced via %s — %04d-%02d-%02d %02d:%02d:%02d (UTC%+d)",
                  server,
                  tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
                  tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
                  utcOffsetHours);
            return;
        }

        log_w(TAG, "NTP server %s failed, trying next...", server);
    }

    log_e(TAG, "All NTP servers failed — continuing without time sync");
}

void WifiManager::disconnect() {
    WiFi.disconnect(/*wifioff=*/true);
    log_i(TAG, "WiFi disconnected");
}

void WifiManager::startAP(const String &ssid) {
    WiFi.disconnect(/*wifioff=*/true); // ensure STA mode is off
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str());        // open network, no password
    log_i(TAG, "SoftAP started: SSID=%s  IP=%s",
          ssid.c_str(), WiFi.softAPIP().toString().c_str());
}
