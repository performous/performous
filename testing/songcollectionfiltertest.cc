#include "common.hh"

#include "game/songcollectionfilter.hh"

#include "game/songfilter.hh"
#include "game/configitem.hh"

namespace {
	struct TestParser : public ISongParser {
		void parse(Song& ) override {}
	};
}

struct UnitTest_SongCollectionFilter : public testing::Test {
	void SetUp() override {
	}

	SongPtr makeSong(std::string const& title, int year = 2000, std::string const& lang = "en", std::string const& artist = "artist") {
		return makeSong(title, artist, year, lang);
	}

	SongPtr makeSong(std::string const& title, std::string const& artist = "artist", int year = 2000, std::string const& lang = "en") {
		auto song = std::make_shared<Song>(m_parser);

		auto filename = fs::path();

		song->filename = filename;
		song->title = title;
		song->artist = artist;
		song->language = lang;
		song->setYear(year);
		song->vocalTracks.emplace("dummy", "dummy");

		return song;
	}

	SongCollection makeCollection() {
		auto songs = SongCollection();

		songs.emplace_back(makeSong("song x", 1991, "en"));
		songs.emplace_back(makeSong("song y", 1995, "de"));
		songs.emplace_back(makeSong("song z", 1981, "fr"));

		return songs;
	}
	
	TestParser m_parser;
};

TEST_F(UnitTest_SongCollectionFilter, empty) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter();
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(3));
}

TEST_F(UnitTest_SongCollectionFilter, type_vocal) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setType(FilterType::HasVocals);
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(3));
}

TEST_F(UnitTest_SongCollectionFilter, type_drums) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setType(FilterType::HasDrumsOrKeyboard);
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(0));
}

TEST_F(UnitTest_SongCollectionFilter, DISABLED_string_x) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("x");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, string_song) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("song");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(3));
}

TEST_F(UnitTest_SongCollectionFilter, filter_language_match_de) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter(std::make_shared<LanguageFilter>("de"));
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, filter_language_not_match_gr) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter(std::make_shared<LanguageFilter>("gr"));
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(0));
}

TEST_F(UnitTest_SongCollectionFilter, filter_year_match_2) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter(std::make_shared<YearFilter>(1990, 1999));
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(2));
}


