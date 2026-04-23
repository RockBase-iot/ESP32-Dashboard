#pragma once

#include <Arduino.h>

// ─── Time / string utilities ──────────────────────────────────────────────

// Format uptime seconds as "Xd Xh Xm Xs".
String formatUptime(uint32_t seconds);

// Format a Unix timestamp using the given strftime format string.
// Uses the system local time (set via SNTP / setenv TZ).
String formatTimestamp(time_t ts, const char *fmt);

// ─── Numeric utilities ────────────────────────────────────────────────────

// Format a float to a string with the given number of decimal places.
String formatFloat(float val, int decimals);

// ─── System utilities ─────────────────────────────────────────────────────

// Return a human-readable reset reason string (e.g. "Power on", "Deep sleep").
String getRebootReason();

// Initialize PSRAM heap allocation when PSRAM is available.
void initPsramHeap();
