#pragma once

#include <stddef.h>
#include <time.h>
#include "app/locale/locale_data.h"

// _strftime — locale-aware strftime replacement.
//
// Behaves like the standard strftime() but sources weekday and month names
// from the provided LocaleData instance instead of the C locale.
//
// This allows runtime language switching without re-flashing: pass the
// LocaleData returned by getLocale() and the formatting stays consistent.
//
// Supported conversion specifiers:
//   %A  Full weekday name (from loc.day[])
//   %a  Abbreviated weekday name (from loc.abday[])
//   %B  Full month name (from loc.mon[])
//   %b  Abbreviated month name (from loc.abmon[])
//   %p  AM/PM string (from loc.am_str / loc.pm_str)
//   All other specifiers are delegated to the standard strftime().
//
// Returns the number of bytes written to buf (excluding the null terminator),
// or 0 if buf is too small.
size_t _strftime(char *buf, size_t buf_size,
                 const char *fmt,
                 const struct tm *tm_info,
                 const LocaleData &loc);
