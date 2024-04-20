#include "game/color.hh"

#include "common.hh"

TEST(UnitTest_Color, default_ctor_white) {
    EXPECT_EQ(1.0f, Color().r);
    EXPECT_EQ(1.0f, Color().g);
    EXPECT_EQ(1.0f, Color().b);
    EXPECT_EQ(1.0f, Color().a);
}

TEST(UnitTest_Color, ctor_rgb) {
    EXPECT_EQ(0.1f, Color(0.1f, 0.2f, 0.3f).r);
    EXPECT_EQ(0.2f, Color(0.1f, 0.2f, 0.3f).g);
    EXPECT_EQ(0.3f, Color(0.1f, 0.2f, 0.3f).b);
    EXPECT_EQ(1.0f, Color(0.1f, 0.2f, 0.3f).a);
}

TEST(UnitTest_Color, ctor_rgba) {
    EXPECT_EQ(0.1f, Color(0.1f, 0.2f, 0.3f, 0.4f).r);
    EXPECT_EQ(0.2f, Color(0.1f, 0.2f, 0.3f, 0.4f).g);
    EXPECT_EQ(0.3f, Color(0.1f, 0.2f, 0.3f, 0.4f).b);
    EXPECT_EQ(0.4f, Color(0.1f, 0.2f, 0.3f, 0.4f).a);
}

TEST(UnitTest_Color, alpha_factory) {
    EXPECT_EQ(1.0f, Color::alpha(0.1f).r);
    EXPECT_EQ(1.0f, Color::alpha(0.1f).g);
    EXPECT_EQ(1.0f, Color::alpha(0.1f).b);
    EXPECT_EQ(0.1f, Color::alpha(0.1f).a);
}

TEST(UnitTest_Color, ctor_ccs_name) {
    auto const colors = std::map<std::string, Color>{
        {"maroon", Color("#800000FF")},
        {"red", Color("#FF0000FF")},
        {"green", Color("#008000FF")},
        {"lime", Color("#00FF00FF")},
        {"navy", Color("#000080FF")},
        {"blue", Color("#0000FFFF")},
        {"purple", Color("#800080FF")},
        {"fuchsia", Color("#FF00FFFF")},
        {"olive", Color("#808000FF")},
        {"yellow", Color("#FFFF00FF")},
        {"teal", Color("#008080FF")},
        {"aqua", Color("#00FFFFFF")},
        {"white", Color("#FFFFFFFF")},
        {"none", Color("#00000000")},
        {"black", Color("#000000FF")},
        {"gray", Color("#808080FF")},
        {"silver", Color("#C0C0C0FF")}
    };

    for (auto const [name, color] : colors)
        EXPECT_EQ(color, Color(name));
}

TEST(UnitTest_Color, ctor_ccs_name_undefined) {
    EXPECT_EQ(Color(1.0f, 0.0f, 1.0f, 1.0f), Color("undefined"));
}

TEST(UnitTest_Color, ctor_ccs_name_upper_case) {
    EXPECT_EQ(Color(1.0f, 0.0f, 1.0f, 1.0f), Color("White"));
    EXPECT_EQ(Color(1.0f, 0.0f, 1.0f, 1.0f), Color("BLACK"));
}

TEST(UnitTest_Color, ctor_ccs_rgb) {
    EXPECT_EQ(Color(1.0f, 0.0f, 1.0f, 1.0f), Color("#FF00FF"));
}

TEST(UnitTest_Color, ctor_ccs_rgba) {
    EXPECT_EQ(Color(1.0f, 0.0f, 1.0f, 0.0f), Color("#FF00FF00"));
}

TEST(UnitTest_Color, ctor_ccs_lower_case) {
    EXPECT_EQ(Color(1.0f, 0.0f, 0.0f, 1.0f), Color("#ff0000ff"));
}

