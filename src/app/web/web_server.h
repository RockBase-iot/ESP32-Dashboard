#pragma once

// WebServer — lightweight HTTP config portal running in Station mode.
//
// Start this AFTER WiFi connects (in STA mode, not AP).
// Serves the single-page frontend at GET /
// REST API:
//   GET  /api/config  → JSON object of all AppConfig fields
//   POST /api/config  → JSON patch (only supplied keys are updated)
class WebServer {
public:
    // Start the HTTP server on port 80. Non-blocking — registers handlers
    // and returns immediately. Call this after WiFi.mode(WIFI_STA) connects.
    void start();

    // Stop the server (call before deep sleep if deep sleep re-enabled).
    void stop();
};
