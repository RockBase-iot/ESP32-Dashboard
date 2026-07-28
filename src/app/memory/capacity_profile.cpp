#include "capacity_profile.h"

namespace {
constexpr size_t kExtendedPsramThreshold = 7UL * 1024UL * 1024UL;
constexpr uint32_t kMinInternalFreeHeap = 80UL * 1024UL;

CapacityProfile safeProfile() {
    return CapacityProfile{
        10,
        6,
        250,
        512UL * 1024UL,
        kMinInternalFreeHeap,
        false,
    };
}

CapacityProfile extendedProfile() {
    return CapacityProfile{
        20,
        6,
        1000,
        2UL * 1024UL * 1024UL,
        kMinInternalFreeHeap,
        true,
    };
}
}  // namespace

CapacityProfile detectCapacity(bool psramFound, size_t psramBytes) {
    if (psramFound && psramBytes >= kExtendedPsramThreshold) {
        return extendedProfile();
    }
    return safeProfile();
}
