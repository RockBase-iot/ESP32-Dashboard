#include "calendar_models.h"

std::string truncateCalendarUtf8(const std::string &value, size_t maxBytes) {
    if (value.size() <= maxBytes) {
        return value;
    }
    size_t end = maxBytes;
    while (end > 0 && (static_cast<unsigned char>(value[end]) & 0xC0U) == 0x80U) {
        --end;
    }
    return value.substr(0, end);
}

std::string unescapeIcsText(const std::string &value) {
    std::string out;
    out.reserve(value.size());
    bool escaping = false;
    for (const char c : value) {
        if (!escaping) {
            if (c == '\\') {
                escaping = true;
            } else {
                out.push_back(c);
            }
            continue;
        }
        switch (c) {
            case 'n':
            case 'N':
                out.push_back('\n');
                break;
            case '\\':
            case ',':
            case ';':
                out.push_back(c);
                break;
            default:
                out.push_back(c);
                break;
        }
        escaping = false;
    }
    if (escaping) {
        out.push_back('\\');
    }
    return out;
}
