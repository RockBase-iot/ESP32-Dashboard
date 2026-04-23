#include "web_config.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "app/config/app_config.h"
#include "utils/logger.h"

static const char *TAG = "WebConfig";

static AsyncWebServer _server(80);
static volatile bool  _saved = false;

void WebConfig::start(const char *ap_ssid) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid);
    log_i(TAG, "AP started: %s  IP=%s", ap_ssid,
          WiFi.softAPIP().toString().c_str());

    // Serve the configuration form.
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        // TODO: serve an HTML form with fields for all AppConfig members.
        req->send(200, "text/html",
                  "<html><body><h1>Weather Station Setup</h1>"
                  "<p>TODO: configuration form</p></body></html>");
    });

    // Handle form submission.
    _server.on("/save", HTTP_POST, [](AsyncWebServerRequest *req) {
        AppConfig cfg;
        // TODO: read form fields from req->getParam(), populate cfg,
        //       validate, and call saveAppConfig(cfg).
        req->send(200, "text/html",
                  "<html><body><p>Saved. Device will restart.</p></body></html>");
        _saved = true;
    });

    _server.begin();

    // Block until config is saved or 5-minute timeout.
    unsigned long start = millis();
    while (!_saved && millis() - start < 5UL * 60UL * 1000UL) {
        delay(100);
    }

    stop();
    if (_saved) {
        log_i(TAG, "Config saved — restarting");
        ESP.restart();
    } else {
        log_w(TAG, "Config portal timed out");
    }
}

void WebConfig::stop() {
    _server.end();
    WiFi.softAPdisconnect(true);
}
