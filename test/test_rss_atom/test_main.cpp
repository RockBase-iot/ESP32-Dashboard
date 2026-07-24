#include <unity.h>

#include "app/news/rss_atom_provider.h"
#include "app/news/rss_atom_provider.cpp"

void test_rss_parser_limits_titles_and_items() {
    std::string xml = "<rss><channel>";
    for (int i = 0; i < 25; ++i) {
        xml += "<item><title>Headline " + std::to_string(i) + "</title><source>Local</source></item>";
    }
    xml += "</channel></rss>";

    const auto parsed = parseRssAtomTitles(xml, 20, 160);

    TEST_ASSERT_EQUAL_UINT32(20, parsed.items.size());
    TEST_ASSERT_EQUAL_STRING("Headline 0", parsed.items[0].title.c_str());
}

void test_atom_parser_unescapes_entities() {
    const auto parsed = parseRssAtomTitles("<feed><entry><title>Markets &amp; policy</title></entry></feed>",
                                           20, 160);

    TEST_ASSERT_EQUAL_UINT32(1, parsed.items.size());
    TEST_ASSERT_EQUAL_STRING("Markets & policy", parsed.items[0].title.c_str());
}

void test_atom_parser_reads_link_href_attribute() {
    const auto parsed = parseRssAtomTitles(
        "<feed><entry><title>Launch</title><source>NASA</source>"
        "<link href=\"https://www.nasa.gov/news/item\" /></entry></feed>",
        20, 160);

    TEST_ASSERT_EQUAL_UINT32(1, parsed.items.size());
    TEST_ASSERT_EQUAL_STRING("https://www.nasa.gov/news/item", parsed.items[0].link.c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_rss_parser_limits_titles_and_items);
    RUN_TEST(test_atom_parser_unescapes_entities);
    RUN_TEST(test_atom_parser_reads_link_href_attribute);
    UNITY_END();
}

void loop() {}
