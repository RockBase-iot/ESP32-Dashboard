#pragma once

#include <stdint.h>

#include <string>

std::string redactKeyValueForLog(const std::string &key, const std::string &value);
bool isAllowedRotationInterval(uint16_t minutes);
bool isSafeDashboardUrl(const std::string &url);
