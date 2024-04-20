#include "game/configitem.hh"

#include "common.hh"

TEST(UnitTest_ConfigItem, i) {
    EXPECT_EQ(0, ConfigItem(0).i());
    EXPECT_EQ(1, ConfigItem(1).i());
    EXPECT_EQ(1000, ConfigItem(1000).i());

    EXPECT_THROW(ConfigItem().i(), std::exception);
    EXPECT_THROW(ConfigItem(static_cast<unsigned short>(0)).i(), std::exception);
    EXPECT_THROW(ConfigItem(true).i(), std::exception);
    EXPECT_THROW(ConfigItem(0.1f).i(), std::exception);
    EXPECT_THROW(ConfigItem(std::string{}).i(), std::exception);
    EXPECT_THROW(ConfigItem(std::vector<std::string>{}).i(), std::exception);
}

TEST(UnitTest_ConfigItem, ui) {
    EXPECT_EQ(0, ConfigItem(static_cast<unsigned short>(0)).ui());
    EXPECT_EQ(1, ConfigItem(static_cast<unsigned short>(1)).ui());
    EXPECT_EQ(1000, ConfigItem(static_cast<unsigned short>(1000)).ui());

    EXPECT_THROW(ConfigItem().ui(), std::exception);
    EXPECT_THROW(ConfigItem(3).ui(), std::exception);
    EXPECT_THROW(ConfigItem(true).ui(), std::exception);
    EXPECT_THROW(ConfigItem(0.1f).ui(), std::exception);
    EXPECT_THROW(ConfigItem(std::string{}).ui(), std::exception);
    EXPECT_THROW(ConfigItem(std::vector<std::string>{}).ui(), std::exception);
}

TEST(UnitTest_ConfigItem, f) {
    EXPECT_EQ(0.0f, ConfigItem(0.0f).f());
    EXPECT_EQ(1.1f, ConfigItem(1.1f).f());
    EXPECT_EQ(1000.0f, ConfigItem(1000.0f).f());

    EXPECT_THROW(ConfigItem().f(), std::exception);
    EXPECT_THROW(ConfigItem(0).f(), std::exception);
    EXPECT_THROW(ConfigItem(static_cast<unsigned short>(0)).f(), std::exception);
    EXPECT_THROW(ConfigItem(true).f(), std::exception);
    EXPECT_THROW(ConfigItem(std::string{}).f(), std::exception);
    EXPECT_THROW(ConfigItem(std::vector<std::string>{}).f(), std::exception);
}

TEST(UnitTest_ConfigItem, b) {
    EXPECT_EQ(true, ConfigItem(true).b());
    EXPECT_EQ(false, ConfigItem(false).b());

    EXPECT_THROW(ConfigItem().b(), std::exception);
    EXPECT_THROW(ConfigItem(0).b(), std::exception);
    EXPECT_THROW(ConfigItem(static_cast<unsigned short>(0)).b(), std::exception);
    EXPECT_THROW(ConfigItem(0.1f).b(), std::exception);
    EXPECT_THROW(ConfigItem(std::string{}).b(), std::exception);
    EXPECT_THROW(ConfigItem(std::vector<std::string>{}).b(), std::exception);
}

TEST(UnitTest_ConfigItem, s) {
    EXPECT_EQ("", ConfigItem(std::string{}).s());
    EXPECT_EQ("Test", ConfigItem(std::string{ "Test" }).s());

    EXPECT_THROW(ConfigItem().s(), std::exception);
    EXPECT_THROW(ConfigItem(false).s(), std::exception);
    EXPECT_THROW(ConfigItem(0).s(), std::exception);
    EXPECT_THROW(ConfigItem(0.1f).s(), std::exception);
    EXPECT_THROW(ConfigItem(std::vector<std::string>{}).s(), std::exception);
}

TEST(UnitTest_ConfigItem, ol) {
    EXPECT_THAT(ConfigItem(std::vector<std::string>{}).ol(), IsEmpty());
    EXPECT_THAT(ConfigItem(std::vector<std::string>{"A"}).ol(), ElementsAre("A"));
    EXPECT_THAT(ConfigItem(std::vector<std::string>{"A", "B", "C"}).ol(), ElementsAre("A", "B", "C"));

    EXPECT_THROW(ConfigItem().ol(), std::exception);
    EXPECT_THROW(ConfigItem(false).ol(), std::exception);
    EXPECT_THROW(ConfigItem(0).ol(), std::exception);
    EXPECT_THROW(ConfigItem(0.1f).ol(), std::exception);
    EXPECT_THROW(ConfigItem(std::string{}).ol(), std::exception);
}

TEST(UnitTest_ConfigItem, so) {
    EXPECT_THROW(ConfigItem(std::vector<std::string>{}).so(), std::exception);
    EXPECT_EQ("A", ConfigItem(std::vector<std::string>{"A"}).so());
    EXPECT_EQ("A", ConfigItem(std::vector<std::string>{"A", "B", "C"}).so());

    EXPECT_THROW(ConfigItem().so(), std::exception);
    EXPECT_THROW(ConfigItem(false).so(), std::exception);
    EXPECT_THROW(ConfigItem(0).so(), std::exception);
    EXPECT_THROW(ConfigItem(0.1f).so(), std::exception);
    EXPECT_THROW(ConfigItem(std::string{}).so(), std::exception);
}

TEST(UnitTest_ConfigItem, select_empty) {
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{}).select(0));
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{}).select(1));
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{}).select(-1));
}

TEST(UnitTest_ConfigItem, select_out_of_range_1) {
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{"A"}).select(1));
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{"A"}).select(-1));

    auto item = ConfigItem(std::vector<std::string>{"A"});

    item.select(1);

    EXPECT_EQ("A", item.so());

    item.select(9);

    EXPECT_EQ("A", item.so());
}

TEST(UnitTest_ConfigItem, select_out_of_range_3) {
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{"A", "B", "C"}).select(3));
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{"A", "B", "C"}).select(-1));

    auto item = ConfigItem(std::vector<std::string>{"A", "B", "C"});

    item.select(3);

    EXPECT_EQ("C", item.so());

    item.select(9);

    EXPECT_EQ("C", item.so());
}

TEST(UnitTest_ConfigItem, select) {
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{"A", "B", "C"}).select(0));
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{"A", "B", "C"}).select(1));
    EXPECT_NO_THROW(ConfigItem(std::vector<std::string>{"A", "B", "C"}).select(2));

    auto item = ConfigItem(std::vector<std::string>{"A", "B", "C"});

    item.select(0);
    EXPECT_EQ("A", item.so());

    item.select(1);
    EXPECT_EQ("B", item.so());

    item.select(2);
    EXPECT_EQ("C", item.so());

    item.select(1);
    EXPECT_EQ("B", item.so());

    item.select(0);
    EXPECT_EQ("A", item.so());
}

TEST(UnitTest_ConfigItem, sl) {
    auto item = ConfigItem(std::vector<std::string>{});

    item.setType("string_list");

    EXPECT_THAT(item.sl(), IsEmpty());

    item.sl() = std::vector<std::string>{ "A" };

    EXPECT_THAT(item.sl(), ElementsAre("A"));

    item.sl() = std::vector<std::string>{ "A", "B", "C" };

    EXPECT_THAT(item.sl(), ElementsAre("A", "B", "C"));
}

TEST(UnitTest_ConfigItem, getType) {
    EXPECT_EQ("bool", ConfigItem(true).getType());
    EXPECT_EQ("int", ConfigItem(2).getType());
    EXPECT_EQ("uint", ConfigItem(static_cast<unsigned short>(4)).getType());
    EXPECT_EQ("float", ConfigItem(1.2f).getType());
    EXPECT_EQ("string", ConfigItem(std::string{}).getType());
    EXPECT_EQ("option_list", ConfigItem(std::vector<std::string>{}).getType());
}

TEST(UnitTest_ConfigItem, getName) {
    EXPECT_EQ("", ConfigItem(true).getName());
}

TEST(UnitTest_ConfigItem, setName) {
    const auto item = []() { auto item = ConfigItem(); item.setName("Test/Item"); return item; }();

    EXPECT_EQ("Test/Item", item.getName());
}

TEST(UnitTest_ConfigItem, getShortDesc) {
    EXPECT_EQ("", ConfigItem().getShortDesc());
}

TEST(UnitTest_ConfigItem, setDescription) {
    const auto item = []() { auto item = ConfigItem(); item.setDescription("Short"); return item; }();

    EXPECT_EQ("Short", item.getShortDesc());
}

TEST(UnitTest_ConfigItem, getLongDesc) {
    EXPECT_EQ("", ConfigItem().getLongDesc());
}

TEST(UnitTest_ConfigItem, setLongDescription) {
    const auto item = []() { auto item = ConfigItem(); item.setLongDescription("Long"); return item; }();

    EXPECT_EQ("Long", item.getLongDesc());
}

TEST(UnitTest_ConfigItem, getEnum_empty) {
    EXPECT_TRUE(ConfigItem().getEnum().empty());
}

TEST(UnitTest_ConfigItem, getEnum_addEnum_1) {
    auto item = ConfigItem(static_cast<unsigned short>(0));

    item.addEnum("A");

    EXPECT_FALSE(item.getEnum().empty());
    EXPECT_EQ(1, item.getEnum().size());
    EXPECT_EQ("A", item.getEnum().at(0));
}

TEST(UnitTest_ConfigItem, getEnum_addEnum_3) {
    auto item = ConfigItem(static_cast<unsigned short>(0));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    EXPECT_FALSE(item.getEnum().empty());
    EXPECT_EQ(3, item.getEnum().size());
    EXPECT_EQ("C", item.getEnum().at(0));
    EXPECT_EQ("A", item.getEnum().at(1));
    EXPECT_EQ("B", item.getEnum().at(2));
}

TEST(UnitTest_ConfigItem, getEnumName_0) {
    auto item = ConfigItem(static_cast<unsigned short>(0));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    EXPECT_EQ("C", item.getEnumName());
}

TEST(UnitTest_ConfigItem, getEnumName_1) {
    auto item = ConfigItem(static_cast<unsigned short>(1));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    EXPECT_EQ("A", item.getEnumName());
}

TEST(UnitTest_ConfigItem, getEnumName_2) {
    auto item = ConfigItem(static_cast<unsigned short>(2));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    EXPECT_EQ("B", item.getEnumName());
}

TEST(UnitTest_ConfigItem, getEnumName_selectEnum) {
    auto item = ConfigItem(static_cast<unsigned short>(0));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    item.selectEnum("B");

    EXPECT_EQ("B", item.getEnumName());
}

TEST(UnitTest_ConfigItem, getSelection) {
    auto item = ConfigItem(static_cast<unsigned short>(0));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    EXPECT_EQ(0, item.getSelection());
}

TEST(UnitTest_ConfigItem, selectEnum_getSelection) {
    auto item = ConfigItem(static_cast<unsigned short>(0));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    item.selectEnum("A");

    EXPECT_EQ(0, item.getSelection());
}

TEST(UnitTest_ConfigItem, increase) {
    auto item = ConfigItem(static_cast<unsigned short>(0));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    EXPECT_EQ(0, item.ui());

    ++item;

    EXPECT_EQ(1, item.ui());

    ++item;

    EXPECT_EQ(2, item.ui());

    ++item;

    EXPECT_EQ(2, item.ui());
}

TEST(UnitTest_ConfigItem, decrease) {
    auto item = ConfigItem(static_cast<unsigned short>(2));

    item.addEnum("C");
    item.addEnum("A");
    item.addEnum("B");

    EXPECT_EQ(2, item.ui());

    --item;

    EXPECT_EQ(1, item.ui());

    --item;

    EXPECT_EQ(0, item.ui());

    --item;

    EXPECT_EQ(0, item.ui());
}

