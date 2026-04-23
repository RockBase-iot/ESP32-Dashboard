#pragma once

// WebConfig — captive-portal / AP-mode configuration web server.
//
// Start the server when no WiFi credentials are available or the user
// holds the boot button. Serves an HTML form; on submit, validates input
// and persists values to NVS via saveAppConfig().
class WebConfig {
public:
    // Start the AP and HTTP server. Blocks until the user saves config
    // or a timeout elapses.
    // ap_ssid: the SSID of the configuration hotspot.
    void start(const char *ap_ssid = "WeatherStation-Setup");

    // Stop the server and AP.
    void stop();
};
