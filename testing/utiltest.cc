#include "common.hh"

#include "game/util.hh"

TEST(UnitTest_Utils, clamp) {
    EXPECT_EQ(0.0, clamp(-1.0, 0.0, 1.0));
    EXPECT_EQ(0.5, clamp(0.5, 0.0, 1.0));
    EXPECT_EQ(1.0, clamp(2.0, 0.0, 1.0));
}

TEST(UnitTest_Utils, smoothstep_1) {
    EXPECT_EQ(0.0, smoothstep(0));
    EXPECT_EQ(0.5, smoothstep(0.5));
    EXPECT_EQ(1.0, smoothstep(1.0));
}

TEST(UnitTest_Utils, smoothstep_1_clamping) {
    EXPECT_EQ(0.0, smoothstep(-1.0));
    EXPECT_EQ(1.0, smoothstep(2.0));
}

TEST(UnitTest_Utils, smoothstep_3) {
    EXPECT_EQ(0.0, smoothstep(-1., 1., -1.0));
    EXPECT_EQ(0.5, smoothstep(-1., 1., 0.0));
    EXPECT_EQ(1.0, smoothstep(-1., 1., 1.0));
}

TEST(UnitTest_Utils, smoothstep_3_clamping) {
    EXPECT_EQ(0.0, smoothstep(-1., 1., -2.0));
    EXPECT_EQ(1.0, smoothstep(-1., 1., 2.0));
}

TEST(UnitTest_Utils, format) {
    auto const time = std::chrono::seconds(7260);

    // next two tests are not working when compiled with mingw. see https://sourceforge.net/p/mingw-w64/bugs/793/
    //EXPECT_EQ("01/01/70 02:01", format(time, "%D %R"));
    //EXPECT_EQ("02:01:00 1970-01-01", format(time, "%T %F"));
    EXPECT_EQ("01/01/70 02:01", format(time, "%m/%d/%y %H:%M", true));
    EXPECT_EQ("02:01:00 1970-01-01", format(time, "%H:%M:%S %Y-%m-%d", true));
    EXPECT_EQ("02:01:00 1970-01-01", format(time, "%X %Y-%m-%d", true));
}

TEST(UnitTest_Utils, replaceFirst_no_hit) {
    EXPECT_EQ("Nothing to replace here", replaceFirst("Nothing to replace here", "hello", "world"));
}

TEST(UnitTest_Utils, replaceFirst_one_hit) {
    EXPECT_EQ("Something to see here", replaceFirst("Something to replace here", "replace", "see"));
}

TEST(UnitTest_Utils, replaceFirst_one_hit_wrong_case) {
    EXPECT_EQ("Something to replace here", replaceFirst("Something to replace here", "Replace", "see"));
}

TEST(UnitTest_Utils, replaceFirst_two_hits) {
    EXPECT_EQ("Something to replace there and here", replaceFirst("Something to replace here and here", "here", "there"));
}

TEST(UnitTest_Utils, make_iterator_range) {
    auto const v = std::vector<int>{ 0, 2, 4, 3, 1 };
    auto const begin = v.begin();
    auto const end = v.end();
    auto const range = make_iterator_range(begin, end);

    auto it = v.begin();

    for(auto const i : range)
        EXPECT_EQ(*it++, i);
}

TEST(UnitTest_Utils, reverse) {
    auto const v = std::vector<int>{ 0, 2, 4, 3, 1 };
    auto it = v.rbegin();

    for(auto const i : reverse(v))
        EXPECT_EQ(*it++, i);
}
