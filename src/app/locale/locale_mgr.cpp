#include "locale_mgr.h"

#include <string.h>

// Forward-declare all language instances.
extern const LocaleData LOCALE_EN_US;
extern const LocaleData LOCALE_ZH_CN;

const LocaleData &getLocale(const char *lang) {
    if (lang == nullptr)              return LOCALE_EN_US;
    if (strcmp(lang, "zh_CN") == 0)  return LOCALE_ZH_CN;
    // Add additional languages here as new locale_XX_XX.cpp files are added.
    return LOCALE_EN_US; // default fallback
}
