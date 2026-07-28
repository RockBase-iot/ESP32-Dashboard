#include "rss_atom_provider.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace {
std::string lowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

std::string unescapeXml(std::string text) {
    struct Entity {
        const char *name;
        const char *value;
    };
    static const Entity entities[] = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&quot;", "\""},
        {"&apos;", "'"},
        {"&#39;", "'"},
    };
    for (const Entity &entity : entities) {
        size_t pos = 0;
        while ((pos = text.find(entity.name, pos)) != std::string::npos) {
            text.replace(pos, std::strlen(entity.name), entity.value);
            pos += std::strlen(entity.value);
        }
    }
    return text;
}

std::string truncateBytes(const std::string &text, size_t maxBytes) {
    if (text.size() <= maxBytes) {
        return text;
    }
    return text.substr(0, maxBytes);
}

bool extractTag(const std::string &text, const std::string &tag, size_t start, size_t end,
                std::string &value) {
    const std::string open = "<" + tag;
    const std::string close = "</" + tag + ">";
    size_t tagStart = lowerAscii(text).find(lowerAscii(open), start);
    while (tagStart != std::string::npos && tagStart < end) {
        const size_t openEnd = text.find('>', tagStart);
        if (openEnd == std::string::npos || openEnd >= end) {
            return false;
        }
        const size_t tagEnd = lowerAscii(text).find(lowerAscii(close), openEnd + 1);
        if (tagEnd == std::string::npos || tagEnd > end) {
            return false;
        }
        value = text.substr(openEnd + 1, tagEnd - openEnd - 1);
        return true;
    }
    return false;
}

bool extractTagAttribute(const std::string &text, const std::string &tag, const std::string &attr,
                         size_t start, size_t end, std::string &value) {
    const std::string lower = lowerAscii(text);
    const std::string open = "<" + lowerAscii(tag);
    const std::string needle = lowerAscii(attr) + "=";
    size_t tagStart = lower.find(open, start);
    while (tagStart != std::string::npos && tagStart < end) {
        const size_t openEnd = text.find('>', tagStart);
        if (openEnd == std::string::npos || openEnd > end) {
            return false;
        }
        const size_t attrPos = lower.find(needle, tagStart);
        if (attrPos != std::string::npos && attrPos < openEnd) {
            const size_t quotePos = attrPos + needle.size();
            if (quotePos >= text.size()) {
                return false;
            }
            const char quote = text[quotePos];
            if (quote != '"' && quote != '\'') {
                return false;
            }
            const size_t valueStart = quotePos + 1;
            const size_t valueEnd = text.find(quote, valueStart);
            if (valueEnd == std::string::npos || valueEnd > openEnd) {
                return false;
            }
            value = text.substr(valueStart, valueEnd - valueStart);
            return true;
        }
        tagStart = lower.find(open, openEnd + 1);
    }
    return false;
}
}  // namespace

RssAtomFeed parseRssAtomTitles(const std::string &xml, size_t maxItems, size_t maxTitleBytes) {
    RssAtomFeed feed;
    const std::string lower = lowerAscii(xml);
    size_t cursor = 0;
    while (feed.items.size() < maxItems) {
        size_t itemStart = lower.find("<item", cursor);
        const size_t entryStart = lower.find("<entry", cursor);
        if (itemStart == std::string::npos || (entryStart != std::string::npos && entryStart < itemStart)) {
            itemStart = entryStart;
        }
        if (itemStart == std::string::npos) {
            break;
        }
        const size_t itemOpenEnd = lower.find('>', itemStart);
        if (itemOpenEnd == std::string::npos) {
            break;
        }
        const std::string closeTag = lower[itemStart + 1] == 'e' ? "</entry>" : "</item>";
        const size_t itemEnd = lower.find(closeTag, itemOpenEnd + 1);
        if (itemEnd == std::string::npos) {
            break;
        }

        RssAtomItem item;
        std::string title;
        if (extractTag(xml, "title", itemOpenEnd + 1, itemEnd, title)) {
            item.title = truncateBytes(unescapeXml(title), maxTitleBytes);
        }
        std::string source;
        if (extractTag(xml, "source", itemOpenEnd + 1, itemEnd, source)) {
            item.source = truncateBytes(unescapeXml(source), 64);
        }
        std::string link;
        if (extractTag(xml, "link", itemOpenEnd + 1, itemEnd, link)) {
            item.link = truncateBytes(unescapeXml(link), 256);
        } else if (extractTagAttribute(xml, "link", "href", itemOpenEnd + 1, itemEnd, link)) {
            item.link = truncateBytes(unescapeXml(link), 256);
        }
        if (!item.title.empty()) {
            feed.items.push_back(item);
        }
        cursor = itemEnd + closeTag.size();
    }
    return feed;
}
