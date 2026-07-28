#pragma once

#include <stdint.h>

#include <vector>

#include "app/calendar/calendar_models.h"

std::vector<uint8_t> encodeCalendarEvents(const std::vector<CalendarEvent> &events);
std::vector<CalendarEvent> decodeCalendarEvents(const std::vector<uint8_t> &bytes);
