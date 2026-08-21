#include "songparserfactory.hh"

#include "song.hh"
#include "songparser-ini.hh"
#include "songparser-sm.hh"
#include "songparser-txt.hh"
#include "songparser-xml.hh"
#include "unicode.hh"
#include "util.hh"

#include <fstream>
#include <sstream>

std::unique_ptr<ISongParser> SongParserFactory::create(Song& song) const {
	std::ifstream f(song.filename.string(), std::ios::binary);
	if (!f.is_open()) {
		throw SongParserException(song, "Could not open song file", 0);
	}
	std::stringstream raw;
	raw << f.rdbuf();
	size_t size = raw.str().length();
	if ((size < 10) || (size > 100000)) {
		throw SongParserException(song, "Does not look like a song file (wrong size)");
	}
	std::string utf8 = UnicodeUtil::convertToUTF8(raw.str(), song.filename.string());
	if (!isText(utf8)) {
		throw SongParserException(song, "Does not look like a song file (binary)");
	}

	if (XmlSongParser::check(raw.str())) {
		song.type = Song::Type::XML; // XMLPP should deal with encoding so we don't have to.
		return std::make_unique<XmlSongParser>(raw.str());
	}
	// SM has to come first as it's very similar in structure to TXT and thus it's possible for SM songs to be erroneously categorized as TXT songs.
	if (SmSongParser::check(utf8)) {
		song.type = Song::Type::SM;
		return std::make_unique<SmSongParser>(utf8);
	}
	if (TxtSongParser::check(utf8)) {
		song.type = Song::Type::TXT;
		return std::make_unique<TxtSongParser>(utf8);
	}
	if (IniSongParser::check(utf8)) {
		song.type = Song::Type::INI;
		return std::make_unique<IniSongParser>(utf8);
	}
	throw SongParserException(song, "Does not look like a song file (wrong header)");
}
