#pragma once

#include "locale_data.h"

// Return the LocaleData instance for the given language code.
// Falls back to en_US for unknown codes.
// lang: "en_US" / "zh_CN" / ...
const LocaleData &getLocale(const char *lang);
