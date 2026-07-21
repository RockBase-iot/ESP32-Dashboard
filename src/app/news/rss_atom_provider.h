#pragma once

#include <stdint.h>

#include <string>
#include <vector>

struct RssAtomItem {
    std::string title;
    std::string source;
    std::string link;
};

struct RssAtomFeed {
    std::vector<RssAtomItem> items;
};

RssAtomFeed parseRssAtomTitles(const std::string &xml, size_t maxItems, size_t maxTitleBytes);
