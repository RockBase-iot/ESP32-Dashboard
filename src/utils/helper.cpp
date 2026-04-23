#include "helper.h"

#include <esp_system.h>
#include <time.h>

String formatUptime(uint32_t seconds) {
    uint32_t d = seconds / 86400;
    uint32_t h = (seconds % 86400) / 3600;
    uint32_t m = (seconds % 3600) / 60;
    uint32_t s = seconds % 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%ud %uh %um %us", d, h, m, s);
    return String(buf);
}

String formatTimestamp(time_t ts, const char *fmt) {
    struct tm tm_info;
    localtime_r(&ts, &tm_info);
    char buf[64];
    strftime(buf, sizeof(buf), fmt, &tm_info);
    return String(buf);
}

String formatFloat(float val, int decimals) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.*f", decimals, val);
    return String(buf);
}

String getRebootReason() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:   return "Power on";
        case ESP_RST_DEEPSLEEP: return "Deep sleep";
        case ESP_RST_SW:        return "Software";
        case ESP_RST_PANIC:     return "Panic";
        case ESP_RST_WDT:       return "Watchdog";
        default:                return "Unknown";
    }
}

void initPsramHeap() {
    if (!psramFound()) {
        // Non-fatal: PSRAM is optional.
        return;
    }
    // Enable PSRAM for heap allocations when available.
    heap_caps_malloc_extmem_enable(0);
}
