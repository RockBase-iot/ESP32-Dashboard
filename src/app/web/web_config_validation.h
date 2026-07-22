#pragma once

#include <stdint.h>

#include <string>

std::string redactKeyValueForLog(const std::string &key, const std::string &value);
bool isAllowedRotationInterval(uint16_t minutes);
bool isSafeDashboardUrl(const std::string &url);

// Hard cap for the power-on config window (seconds).
inline constexpr uint16_t kPortalWindowSecMax = 600;

// Clamp a user-provided power-on config window: <= 0 disables it (offline
// light-sleep profile), anything above kPortalWindowSecMax is clamped.
uint16_t normalizePortalWindowSec(int32_t seconds);
