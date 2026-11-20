#include "common.hh"

#include "game/song.hh"
#include "game/songparser-xml.hh"

TEST(UnitTest_XmlSongParser, missing_tempo_attribute_is_a_parser_error_not_a_crash) {
	// Used to dereference the null result of get_attribute("Tempo") during the library scan.
	const std::string xml =
		"<?xml version=\"1.0\"?>\n"
		"<!--Artist: a-->\n"
		"<!--Title: b-->\n"
		"<MELODY Resolution=\"Semiquaver\"/>\n";
	Song song;
	EXPECT_THROW(XmlSongParser(xml).parse(song), SongParserException);
}
