#include "common.hh"

#include "game/songcollectionfilter.hh"

#include "game/songfilter.hh"
#include "game/configitem.hh"
#include "game/i18n.hh"


struct UnitTest_SongCollectionFilter : public testing::Test {
	void SetUp() override {
		TranslationEngine::setLanguage("en", true);
	}

	SongPtr makeSong(std::string const& title, int year = 2000, std::string const& lang = "en", std::string const& artist = "unknown") {
		return makeSong(title, artist, year, lang);
	}

	SongPtr makeSong(std::string const& title, std::string const& artist = "unknown", int year = 2000, std::string const& lang = "en") {
		auto song = std::make_shared<Song>();

		auto filename = fs::path();

		song->filename = filename;
		song->title = title;
		song->artist = artist;
		song->language = lang;
		song->setYear(year);
		song->vocalTracks.emplace("dummy", "dummy");

		return song;
	}

	SongPtr makeDance(std::string const& title, int year = 2000, std::string const& lang = "en", std::string const& artist = "unknown") {
		return makeDance(title, artist, year, lang);
	}

	SongPtr makeDance(std::string const& title, std::string const& artist = "unknown", int year = 2000, std::string const& lang = "en") {
		auto song = std::make_shared<Song>();

		auto filename = fs::path();

		song->filename = filename;
		song->title = title;
		song->artist = artist;
		song->language = lang;
		song->setYear(year);
		song->danceTracks["dummy"] = { };

		return song;
	}

	SongCollection makeCollection() {
		auto songs = SongCollection();

		songs.emplace_back(makeSong("song one 1", 1991, "en"));
		songs.emplace_back(makeSong("song two 2", 1995, "de"));
		songs.emplace_back(makeSong("song three 3", 1981, "fr"));
		songs.emplace_back(makeDance("dance first", 1999, "dk"));

		return songs;
	}
};

TEST_F(UnitTest_SongCollectionFilter, empty) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter();
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(4));
}

TEST_F(UnitTest_SongCollectionFilter, type_vocal) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setType(FilterType::HasVocals);
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(3));
}

TEST_F(UnitTest_SongCollectionFilter, type_dance) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setType(FilterType::HasDance);
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, type_drums) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setType(FilterType::HasDrumsOrKeyboard);
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(0));
}

TEST_F(UnitTest_SongCollectionFilter, string_1) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("1");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, string_t) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("t");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(3));
}

TEST_F(UnitTest_SongCollectionFilter, string_tw) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("tw");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, string_two) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("two");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, string_thre) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("thre");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, string_three) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("three");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, string_song) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("song");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(3));
}

TEST_F(UnitTest_SongCollectionFilter, string_song_two_2) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("song two 2");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(1));
}

TEST_F(UnitTest_SongCollectionFilter, string_nonexistent) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("nonexistent");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(0));
}

TEST_F(UnitTest_SongCollectionFilter, string_unknown) {
	auto songs = makeCollection();
	auto const filter = SongCollectionFilter().setFilter("unknown");
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(4));
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
	auto const filter = SongCollectionFilter().setFilter(std::make_shared<YearFilter>(1990, 1998));
	auto const result = filter.filter(songs);

	EXPECT_THAT(result.size(), Eq(2));
}


