#pragma once

// DashboardApp — top-level application controller.
//
// Called once from setup(). Orchestrates:
//   1. Load config from NVS.
//   2. Initialize board hardware.
//   3. Show loading page.
//   4. Connect WiFi + sync time (SNTP).
//   5. Fetch weather and AQI.
//   6. Render weather page.
//   7. Enter deep sleep.
class DashboardApp {
public:
    // Entry point — never returns (ends with deep sleep).
    static void run();
};
