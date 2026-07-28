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
    std::string statusText;
    int dayDelta = 0;
};

struct WorldClockModel {
    std::vector<WorldClockSlot> clocks;
    std::string focusLabel;
    std::string focusText;
};

WorldClockConfig sampleWorldClockConfig();
WorldClockConfig worldClockConfigFromZonesText(const std::string &zonesText,
                                               const std::string &focusLabel = "",
                                               bool twentyFourHour = true);
WorldClockModel buildWorldClock(const WorldClockConfig &config, int64_t nowUtc);
std::string formatWorldClockDateLabel(int64_t nowUtc, const std::string &timezoneId);
std::string formatWorldClockChromeTimeLabel(int64_t nowUtc, const std::string &timezoneId,
                                            bool twentyFourHour = true);
