#include "_strftime.h"

#include <string.h>
#include <stdio.h>

size_t _strftime(char *buf, size_t buf_size,
                 const char *fmt,
                 const struct tm *tm_info,
                 const LocaleData &loc) {
    if (!buf || buf_size == 0 || !fmt || !tm_info) return 0;

    char out[256] = {};
    size_t out_len = 0;

    for (const char *p = fmt; *p != '\0' && out_len < buf_size - 1; ++p) {
        if (*p != '%') {
            out[out_len++] = *p;
            continue;
        }
        ++p;
        if (*p == '\0') break;

        const char *insert = nullptr;
        char tmp[64] = {};

        switch (*p) {
            case 'A': // Full weekday name
                insert = loc.day[tm_info->tm_wday % 7];
                break;
            case 'a': // Abbreviated weekday name
                insert = loc.abday[tm_info->tm_wday % 7];
                break;
            case 'B': // Full month name
                insert = loc.mon[tm_info->tm_mon % 12];
                break;
            case 'b': // Abbreviated month name
                insert = loc.abmon[tm_info->tm_mon % 12];
                break;
            case 'p': // AM / PM
                insert = (tm_info->tm_hour < 12) ? loc.am_str : loc.pm_str;
                break;
            default:
                // Delegate to standard strftime for all other specifiers.
                {
                    char spec[3] = { '%', *p, '\0' };
                    strftime(tmp, sizeof(tmp), spec, tm_info);
                    insert = tmp;
                }
                break;
        }

        if (insert) {
            size_t ins_len = strlen(insert);
            if (out_len + ins_len >= buf_size) {
                // Not enough space — truncate gracefully.
                strncpy(out + out_len, insert, buf_size - out_len - 1);
                out_len = buf_size - 1;
                break;
            }
            memcpy(out + out_len, insert, ins_len);
            out_len += ins_len;
        }
    }

    out[out_len] = '\0';
    memcpy(buf, out, out_len + 1);
    return out_len;
}
