#pragma once

#include <Arduino.h>

// WifiManager — connect to an AP, sync time via SNTP, and disconnect.
class WifiManager {
public:
    // Connect to the given SSID. Returns true when an IP is obtained.
    // Blocks until connected or a timeout (~15 s) is reached.
    bool connect(const String &ssid, const String &password);

    // Synchronize the system clock via SNTP.
    // tz_string is a POSIX TZ string, e.g. "EST5EDT,M3.2.0,M11.1.0".
    void syncTime(const char *tz_string);

    // Disconnect WiFi and power down the radio.
    void disconnect();
};
