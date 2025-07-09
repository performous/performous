#include "game/microphones.hh"

#include "common.hh"

MATCHER_P(ColorNameIs, n, "") {
    *result_listener << "where the color name is " << n; return arg.first == n; 
}

TEST(UnitTest_Microphones, getMicrophoneColor_blue) {
    auto const sut = getMicrophoneColor("blue");

    EXPECT_NEAR(0.0f, sut.r, 0.0001f);
    EXPECT_NEAR(0.171569f, sut.g, 0.0001f);
    EXPECT_NEAR(1.0f, sut.b, 0.0001f);
    EXPECT_EQ(1.0f, sut.a);
}

TEST(UnitTest_Microphones, getMicrophoneColor_red) {
    auto const sut = getMicrophoneColor("red");

    EXPECT_NEAR(1.0f, sut.r, 0.0001f);
    EXPECT_NEAR(0.0f, sut.g, 0.0001f);
    EXPECT_NEAR(0.0f, sut.b, 0.0001f);
    EXPECT_EQ(1.0f, sut.a);
}

TEST(UnitTest_Microphones, getMicrophoneColor_white) {
    auto const sut = getMicrophoneColor("white");

    EXPECT_NEAR(1.0f, sut.g, 0.0001f);
    EXPECT_NEAR(1.0f, sut.r, 0.0001f);
    EXPECT_NEAR(1.0f, sut.b, 0.0001f);
    EXPECT_EQ(1.0f, sut.a);
}

TEST(UnitTest_Microphones, getMicrophoneColor_black) {
    auto const sut = getMicrophoneColor("black");

    EXPECT_NEAR(0.0117647f, sut.g, 0.0001f);
    EXPECT_NEAR(0.0117647f, sut.r, 0.0001f);
    EXPECT_NEAR(0.0117647f, sut.b, 0.0001f);
    EXPECT_EQ(1.0f, sut.a);
}

TEST(UnitTest_Microphones, getMicrophoneColor_unknown) {
    auto const sut = getMicrophoneColor("unknown");

    EXPECT_NEAR(0.5f, sut.r, 0.0001f);
    EXPECT_NEAR(0.5f, sut.g, 0.0001f);
    EXPECT_NEAR(0.5f, sut.b, 0.0001f);
    EXPECT_EQ(1.0f, sut.a);
}

TEST(UnitTest_Microphones, getMicrophoneConfig_sequence) {
    auto const sut = getMicrophoneConfig();

    EXPECT_THAT(sut, UnorderedElementsAre(
        ColorNameIs("blue"),
        ColorNameIs("red"),
        ColorNameIs("green"),
        ColorNameIs("yellow"),
        ColorNameIs("fuchsia"),
        ColorNameIs("orange"),
        ColorNameIs("purple"),
        ColorNameIs("aqua"),
        ColorNameIs("white"),
        ColorNameIs("gray"),
        ColorNameIs("black")
    ));
}
