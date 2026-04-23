#include "app/dashboardApp.h"

void setup() {
    DashboardApp::run();
}

void loop() {
    // Never reached: DashboardApp::run() ends with deep sleep.
}
