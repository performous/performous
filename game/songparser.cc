#include "songparser.hh"
#include "unicode.hh"
#include "i18n.hh"
#include "util.hh"

#include <boost/algorithm/string.hpp>
#include <fmt/format.h>

#include <fstream>

void SongParser::parse(Song& song) {
	try {
		// Read the file, determine the type and do some initial validation checks
		std::ifstream f(song.filename.string(), std::ios::binary);
		if (!f.is_open()) {
			throw SongParserException(song, "Could not open song file", 0);
		}
		m_ss << f.rdbuf();
		size_t size = m_ss.str().length();
		if ((size < 10) || (size > 100000)) {
			throw SongParserException(song, "Does not look like a song file (wrong size)");
		}
		std::string ss = UnicodeUtil::convertToUTF8(m_ss.str(), song.filename.string());
		if (!isText(ss)) {
			throw SongParserException(song, "Does not look like a song file (binary)");
		}
		// Convert m_ss; filename supplied for possible warning messages
		if (xmlCheck(m_ss.str())) {
			song.type = Song::Type::XML; // XMLPP should deal with encoding so we don't have to.
			ss = m_ss.str();
		}
		else {
			// For determining song type, SM has to come first as it's very similar in structure to the TXT format and thus it's possible for SM songs to be erroneously categorized as TXT songs.
			if (smCheck(ss)) {
				song.type = Song::Type::SM;
			} else if (txtCheck(ss)) {
				song.type = Song::Type::TXT;
			} else if (iniCheck(ss)) {
				song.type = Song::Type::INI;
			} else {
				throw SongParserException(song, "Does not look like a song file (wrong header)");
			}
			m_ss.str(ss);
		}
		// Header always parsed after this point
		bool headerAlreadyParsed = song.loadStatus == Song::LoadStatus::HEADER;
		if (!headerAlreadyParsed) {
			// Parse only header to speed up loading and conserve memory
			if (song.type == Song::Type::TXT) txtParseHeader(song);
			else if (song.type == Song::Type::INI) iniParseHeader(song);
			else if (song.type == Song::Type::XML) xmlParseHeader(song);
			else if (song.type == Song::Type::SM) {
				smParseHeader(song); song.dropNotes();  // Hack: drop notes here (load again when playing the song)
			}
		}

		SongParserUtil::guessFiles(song);

		if (headerAlreadyParsed) {
			if (!song.m_bpms.empty()) {
				float bpm = static_cast<float>(15.0 / song.m_bpms.front().step);
				song.m_bpms.clear();
				SongParserUtil::addBPM(song, 0, bpm, m_gap);
			}
			if (song.type == Song::Type::TXT) txtParse(song);
			else if (song.type == Song::Type::INI) midParse(song);  // INI doesn't contain notes, parse those from MIDI
			else if (song.type == Song::Type::XML) xmlParse(song);
			else if (song.type == Song::Type::SM) smParse(song);
			SongParserUtil::finalize(song, m_tsPerBeat, m_tsEnd, m_gap);  // Do some adjusting to the notes
			song.loadStatus = Song::LoadStatus::FULL;
			return;
		}
		if (!song.midifilename.empty()) {
			midParseHeader(song);
		}
		if (song.loadStatus != Song::LoadStatus::PARSERERROR) {
			song.loadStatus = Song::LoadStatus::HEADER;
		}
	}
	catch (SongParserException&) {
		throw;
	}
	catch (std::exception& e) {
		throw SongParserException(song, fmt::format("Caught exception={}", e.what()), m_linenum, false);
	}
}

