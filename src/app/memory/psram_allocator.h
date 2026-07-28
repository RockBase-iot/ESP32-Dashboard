#pragma once

#include <stddef.h>
#include <stdlib.h>

#if defined(ARDUINO)
#include <esp_heap_caps.h>
#endif

inline void *allocateDashboardBuffer(size_t bytes) {
#if defined(ARDUINO)
    void *ptr = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (ptr) return ptr;
#endif
    return malloc(bytes);
}

inline void freeDashboardBuffer(void *ptr) {
    free(ptr);
}
