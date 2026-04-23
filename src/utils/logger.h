#pragma once

#include <Arduino.h>

// ─── Log level constants ──────────────────────────────────────────────────
// Set DEBUG_LEVEL in build_flags to control verbosity:
//   0  = Info, Warning, Error only  (default / production)
//   1  = + Debug messages
//   2  = + Verbose messages
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

// ─── ANSI color codes (Serial output) ────────────────────────────────────
#define LOG_COLOR_RESET  "\033[0m"
#define LOG_COLOR_RED    "\033[1;31m"   // bright red
#define LOG_COLOR_YELLOW "\033[1;33m"   // bright yellow
#define LOG_COLOR_CYAN   "\033[1;36m"   // bright cyan
#define LOG_COLOR_WHITE  "\033[1;32m"   // bright green  (more visible than white)

// ─── Internal formatting helper ───────────────────────────────────────────
// LOG_SHOW_LOCATION: set to 1 to print file:line in every log line.
// Set to 0 for production to keep output clean.
#ifndef LOG_SHOW_LOCATION
#define LOG_SHOW_LOCATION 0
#endif

#if LOG_SHOW_LOCATION
#define _LOG(color, level, tag, fmt, ...)                                    \
    Serial.printf(color "[%s][" level "] %s:%d " fmt LOG_COLOR_RESET "\n",  \
                  (tag), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define _LOG(color, level, tag, fmt, ...)                                    \
    Serial.printf(color "[%s][" level "] " fmt LOG_COLOR_RESET "\n",        \
                  (tag), ##__VA_ARGS__)
#endif

// ─── Public macros ────────────────────────────────────────────────────────
// Undef Arduino-ESP32 built-in log macros so our tag-aware versions take over.
#ifdef log_e
#undef log_e
#endif
#ifdef log_w
#undef log_w
#endif
#ifdef log_i
#undef log_i
#endif
#ifdef log_d
#undef log_d
#endif
#ifdef log_v
#undef log_v
#endif

#define log_e(tag, fmt, ...) _LOG(LOG_COLOR_RED,    "E", tag, fmt, ##__VA_ARGS__)
#define log_w(tag, fmt, ...) _LOG(LOG_COLOR_YELLOW, "W", tag, fmt, ##__VA_ARGS__)
#define log_i(tag, fmt, ...) _LOG(LOG_COLOR_WHITE,  "I", tag, fmt, ##__VA_ARGS__)

#if DEBUG_LEVEL >= 1
#define log_d(tag, fmt, ...) _LOG(LOG_COLOR_CYAN,   "D", tag, fmt, ##__VA_ARGS__)
#else
#define log_d(tag, fmt, ...) do {} while (0)
#endif

#if DEBUG_LEVEL >= 2
#define log_v(tag, fmt, ...) _LOG(LOG_COLOR_RESET,  "V", tag, fmt, ##__VA_ARGS__)
#else
#define log_v(tag, fmt, ...) do {} while (0)
#endif
