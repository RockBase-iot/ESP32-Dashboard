#include "wifi_manager.h"

#include <WiFi.h>
#include <esp_sntp.h>
#include "utils/logger.h"

static const char *TAG     = "WifiManager";
static const char *NTP_SVR = "pool.ntp.org";
static const int   CONNECT_TIMEOUT_MS = 15000;

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

void WifiManager::syncTime(const char *tz_string) {
    configTime(0, 0, NTP_SVR);
    setenv("TZ", tz_string, 1);
    tzset();

    // Wait for SNTP sync (up to 10 s).
    struct tm tm_info;
    unsigned long start = millis();
    while (!getLocalTime(&tm_info)) {
        if (millis() - start > 10000) {
            log_w(TAG, "SNTP sync timed out");
            return;
        }
        delay(200);
    }
    log_i(TAG, "Time synced: %04d-%02d-%02d %02d:%02d:%02d",
          tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
          tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
}

void WifiManager::disconnect() {
    WiFi.disconnect(/*wifioff=*/true);
    log_i(TAG, "WiFi disconnected");
}
