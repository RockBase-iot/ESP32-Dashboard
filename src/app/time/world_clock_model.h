#pragma once

#include <stdint.h>

#include <string>
#include <vector>

struct WorldClockZone {
    std::string label;
    std::string timezoneId;
};

struct WorldClockConfig {
    bool twentyFourHour = true;
    std::vector<WorldClockZone> zones;
    std::string focusLabel;
    std::string focusText;
};

struct WorldClockSlot {
    std::string label;
    std::string timezoneId;
    std::string timeText;
    std::string dateText;
    int dayDelta = 0;
};

struct WorldClockModel {
    std::vector<WorldClockSlot> clocks;
    std::string focusLabel;
    std::string focusText;
};

WorldClockConfig sampleWorldClockConfig();
WorldClockModel buildWorldClock(const WorldClockConfig &config, int64_t nowUtc);
std::string formatWorldClockDateLabel(int64_t nowUtc, const std::string &timezoneId);
