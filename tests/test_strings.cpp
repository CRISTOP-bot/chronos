#include <gtest/gtest.h>
#include "chronos/memory/strings.h"
#include <cstring>

using namespace chronos;
using namespace chronos::memory;

TEST(Strings, ExtractBasic) {
    std::vector<u8> data;
    // "hello" followed by null
    for (char c : std::string("hello")) data.push_back((u8)c);
    data.push_back(0);
    // short string (below min_len=4) — "hi" should not appear
    for (char c : std::string("hi")) data.push_back((u8)c);
    data.push_back(0);
    // "world" — long enough
    for (char c : std::string("world")) data.push_back((u8)c);
    data.push_back(0);

    auto strs = extract_strings(data, 4);
    ASSERT_EQ(strs.size(), 2u);
    EXPECT_EQ(strs[0], "hello");
    EXPECT_EQ(strs[1], "world");
}

TEST(Strings, MinLenRespected) {
    std::vector<u8> data;
    for (char c : std::string("ab")) data.push_back((u8)c);
    data.push_back(0);
    for (char c : std::string("abcd")) data.push_back((u8)c);
    data.push_back(0);

    auto strs4 = extract_strings(data, 4);
    ASSERT_EQ(strs4.size(), 1u);
    EXPECT_EQ(strs4[0], "abcd");

    auto strs2 = extract_strings(data, 2);
    ASSERT_EQ(strs2.size(), 2u);
    EXPECT_EQ(strs2[0], "ab");
    EXPECT_EQ(strs2[1], "abcd");
}

TEST(Strings, NonPrintableDelimits) {
    // Bytes below 32 or >= 127 terminate a string
    std::vector<u8> data = {
        'c','h','r','o','n','o','s',  // "chronos"
        0x01,                          // non-printable delimiter
        'x','y','z','w',              // "xyzw"
        0x80,                          // >= 127 delimiter
    };
    auto strs = extract_strings(data, 4);
    ASSERT_EQ(strs.size(), 2u);
    EXPECT_EQ(strs[0], "chronos");
    EXPECT_EQ(strs[1], "xyzw");
}

TEST(Strings, EmptyData) {
    std::vector<u8> empty;
    EXPECT_TRUE(extract_strings(empty).empty());
}

TEST(Strings, AllNonPrintable) {
    std::vector<u8> data = {0x00, 0x01, 0x1f, 0x80, 0xff};
    EXPECT_TRUE(extract_strings(data).empty());
}

TEST(Strings, TrailingStringWithoutTerminator) {
    // String at end of buffer with no null terminator should still be extracted
    std::vector<u8> data;
    for (char c : std::string("trailing")) data.push_back((u8)c);
    // no null at end
    auto strs = extract_strings(data, 4);
    ASSERT_EQ(strs.size(), 1u);
    EXPECT_EQ(strs[0], "trailing");
}
