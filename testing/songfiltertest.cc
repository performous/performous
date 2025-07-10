#include "common.hh"

#include "game/songfilter.hh"
#include "game/song.hh"

namespace {
	Song createSong(int year) {
		auto&& song = Song();

		song.setYear(year);

		return song;
	}

	Song createSong(std::string const& title, std::string const& artist = {}, std::string const& language = {}, int year = 2000) {
		auto&& song = Song();

		song.title = title;
		song.artist = artist;
		song.language = language;
		song.setYear(year);

		return song;
	}
}

TEST(UnitTest_YearFilter, no_match_too_low) {
	auto const filter = YearFilter(2000, 2009);
	auto const song = createSong(1999);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_YearFilter, no_match_too_high) {
	auto const filter = YearFilter(2000, 2009);
	auto const song = createSong(2010);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_YearFilter, match_at_lower_bound) {
	auto const filter = YearFilter(2000, 2009);
	auto const song = createSong(2000);

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_YearFilter, match_at_upper_bound) {
	auto const filter = YearFilter(2000, 2009);
	auto const song = createSong(2009);

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_YearFilter, match_mid) {
	auto const filter = YearFilter(2000, 2009);
	auto const song = createSong(2005);

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_LanguageFilter, no_match) {
	auto const filter = LanguageFilter("labc");
	auto const song = createSong({}, {}, "ldef");

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_LanguageFilter, match) {
	auto const filter = LanguageFilter("labc");
	auto const song = createSong({}, {}, "labc");

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_TitleFilter, no_match) {
	auto const filter = TitleFilter("tabc");
	auto const song = createSong("tdef");

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_TitleFilter, match) {
	auto const filter = TitleFilter("tabc");
	auto const song = createSong("tabc");

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_ArtistFilter, no_match) {
	auto const filter = ArtistFilter("aabc");
	auto const song = createSong({}, "adef");

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_ArtistFilter, match) {
	auto const filter = ArtistFilter("aabc");
	auto const song = createSong({}, "aabc");

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_OrFilter, not_match_empty) {
	auto const filter = OrFilter();
	auto const song = createSong({}, "aabc");

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_OrFilter, not_match_one_no_match) {
	auto const filter = OrFilter().add(std::make_shared<YearFilter>(2000, 2009));
	auto const song = createSong(1999);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_OrFilter, match_one_match) {
	auto const filter = OrFilter().add(std::make_shared<YearFilter>(2000, 2009));
	auto const song = createSong(2000);

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_OrFilter, not_match_two_no_match) {
	auto const filter = OrFilter()
		.add(std::make_shared<YearFilter>(2000, 2009))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(1990);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_OrFilter, match_two_first_match) {
	auto const filter = OrFilter()
		.add(std::make_shared<YearFilter>(2000, 2009))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(2000);

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_OrFilter, match_two_second_match) {
	auto const filter = OrFilter()
		.add(std::make_shared<YearFilter>(2000, 2009))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(2010);

	EXPECT_TRUE(filter.filter(song));
}


TEST(UnitTest_OrFilter, match_two_both_match) {
	auto const filter = OrFilter()
		.add(std::make_shared<YearFilter>(2000, 2010))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(2010);

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_AndFilter, match_empty) {
	auto const filter = AndFilter();
	auto const song = createSong({}, "aabc");

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_AndFilter, not_match_one_no_match) {
	auto const filter = AndFilter().add(std::make_shared<YearFilter>(2000, 2009));
	auto const song = createSong(1999);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_AndFilter, match_one_match) {
	auto const filter = AndFilter().add(std::make_shared<YearFilter>(2000, 2009));
	auto const song = createSong(2000);

	EXPECT_TRUE(filter.filter(song));
}

TEST(UnitTest_AndFilter, not_match_two_no_match) {
	auto const filter = AndFilter()
		.add(std::make_shared<YearFilter>(2000, 2009))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(1990);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_AndFilter, not_match_two_first_match) {
	auto const filter = AndFilter()
		.add(std::make_shared<YearFilter>(2000, 2009))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(2000);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_AndFilter, not_match_two_second_match) {
	auto const filter = AndFilter()
		.add(std::make_shared<YearFilter>(2000, 2009))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(2010);

	EXPECT_FALSE(filter.filter(song));
}

TEST(UnitTest_AndFilter, match_two_both_match) {
	auto const filter = AndFilter()
		.add(std::make_shared<YearFilter>(2000, 2010))
		.add(std::make_shared<YearFilter>(2010, 2019));
	auto const song = createSong(2010);

	EXPECT_TRUE(filter.filter(song));
}
