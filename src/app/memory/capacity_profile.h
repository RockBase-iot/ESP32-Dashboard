#pragma once

#include <stddef.h>
#include <stdint.h>

struct CapacityProfile {
    uint8_t calendarSlots;
    uint8_t defaultEnabledSources;
    uint16_t maxExpandedEvents;
    uint32_t maxSourceBytes;
    uint32_t minInternalFreeHeap;
    bool extended;
};

CapacityProfile detectCapacity(bool psramFound, size_t psramBytes);
