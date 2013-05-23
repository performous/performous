#include "songparser.hh"

#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>


namespace SongParserUtil {
	void assign(int& var, std::string const& str) {
		try {
			var = boost::lexical_cast<int>(str);
		} catch (...) {
			throw std::runtime_error("\"" + str + "\" is not valid integer value");
		}
	}
	void assign(unsigned& var, std::string const& str) {
		try {
			var = boost::lexical_cast<int>(str);
		} catch (...) {
			throw std::runtime_error("\"" + str + "\" is not valid unsigned integer value");
		}
	}
	void assign(double& var, std::string str) {
		std::replace(str.begin(), str.end(), ',', '.'); // Fix decimal separators
		try {
			var = boost::lexical_cast<double>(str);
		} catch (...) {
			throw std::runtime_error("\"" + str + "\" is not valid floating point value");
		}
	}
	void assign(bool& var, std::string const& str) {
		if (str == "YES" || str == "yes" || str == "1") var = true;
		else if (str == "NO" || str == "no" || str == "0") var = false;
		else throw std::runtime_error("Invalid boolean value: " + str);
	}
	void eraseLast(std::string& s, char ch) {
		if (!s.empty() && *s.rbegin() == ch) s.erase(s.size() - 1);
	}
}


/// constructor
SongParser::SongParser(Song& s) try:
  m_song(s),
  m_linenum(),
  m_relative(),
  m_gap(),
  m_bpm(),
  m_prevtime(),
  m_prevts(),
  m_relativeShift(),
  m_tsPerBeat(),
  m_tsEnd()
{
	enum { NONE, TXT, XML, INI, SM } type = NONE;
	// Read the file, determine the type and do some initial validation checks
	{
		std::ifstream f(s.filename.c_str(), std::ios::binary);
		if (!f.is_open()) throw SongParserException(s, "Could not open song file", 0);
		f.seekg(0, std::ios::end);
		size_t size = f.tellg();
		if (size < 10 || size > 100000) throw SongParserException(s, "Does not look like a song file (wrong size)", 1, true);
		f.seekg(0);
		std::vector<char> data(size);
		if (!f.read(&data[0], size)) throw SongParserException(s, "Unexpected I/O error", 0);
		if (smCheck(data)) type = SM;
		else if (txtCheck(data)) type = TXT;
		else if (iniCheck(data)) type = INI;
		else if (xmlCheck(data)) type = XML;
		else throw SongParserException(s, "Does not look like a song file (wrong header)", 1, true);
		m_ss.write(&data[0], size);
	}
	// Convert m_ss; filename supplied for possible warning messages
	convertToUTF8(m_ss, s.filename.string());
	// Header already parsed?
	if (s.loadStatus == Song::HEADER) {
		if (type == TXT) txtParse();
		else if (type == INI) iniParse();
		else if (type == XML) xmlParse();
		else if (type == SM) smParse();
		finalize(); // Do some adjusting to the notes
		s.loadStatus = Song::FULL;
		return;
	}
	// Parse only header to speed up loading and conserve memory
	if (type == TXT) txtParseHeader();
	else if (type == INI) iniParseHeader();
	else if (type == XML) xmlParseHeader();
	else if (type == SM) { smParseHeader(); s.dropNotes(); } // Hack: drop notes here
	// Default for preview position if none was specified in header
	if (s.preview_start != s.preview_start) s.preview_start = (type == INI ? 5.0 : 30.0);  // 5 s for band mode, 30 s for others

	guessFiles();

	s.loadStatus = Song::HEADER;
} catch (std::runtime_error& e) {
	throw SongParserException(m_song, e.what(), m_linenum);
} catch (std::exception& e) {
	throw SongParserException(m_song, "Internal error: " + std::string(e.what()), m_linenum);
}

void SongParser::guessFiles() {
	// List of fields containing filenames, and auto-matching regexps
	std::vector<std::pair<fs::path*, char const*>> fields = {
		{ &m_song.cover, R"((cover|album|label|\[co\])\.(png|jpeg|jpg|svg)$)" },
		{ &m_song.background, R"(\.(png|jpeg|jpg|svg)$)" },
		{ &m_song.video, R"(\.(avi|mpg|mpeg|flv|mov|mp4)$)" },
		{ &m_song.midifilename, R"(\.(mid)$)" },
	};
	//if (m_song.music.size() == 1) fields.emplace_back(&m_song.music.begin()->second, R"(\.(mp3|ogg)$)");

	std::string logMissing, logFound;
	
	// Run checks, remove bogus values and construct regexps
	namespace fs = boost::filesystem;
	std::vector<boost::regex> regexps;
	bool missing = false;
	for (auto const& p: fields) {
		fs::path& file = *p.first;
		if (!file.empty() && !is_regular_file(file)) {
			logMissing += " \"" + file.filename().string() + "\"";
			file.clear();
		}
		if (file.empty()) missing = true;
		regexps.emplace_back(p.second, boost::regex_constants::icase);
	}

	if (!missing) return;  // All OK!
		
	// Scan the entire folder for matching files
	for (fs::directory_iterator dirIt(m_song.path), dirEnd; dirIt != dirEnd; ++dirIt) {
		std::string name = dirIt->path().filename().string(); // File basename
		for (unsigned i = 0; i < fields.size(); ++i) {
			auto& field = *fields[i].first;
			if (!field.empty()) continue;  // Valid filename is already known
			if (!regex_search(name, regexps[i])) continue;  // No match for current file
			field = dirIt->path();
			logFound += " \"" + name + "\"";
			goto next_file;  // Necessary for intelligent cover/bg detection
		}
		next_file:;
	}
	
	if (logFound.empty() && logMissing.empty()) return;
	std::clog << "songparser/warning: \"" + m_song.path.string() + "\":";
	if (!logMissing.empty()) std::clog << logMissing + " not found  ";
	if (!logFound.empty()) std::clog << logFound + " autodetected";
	std::clog << std::endl;

/*
	// Search the dir for the music files
	for (boost::filesystem::directory_iterator dirIt(s.path), dirEnd; dirIt != dirEnd; ++dirIt) {
		boost::filesystem::path p = dirIt->path();
		std::string name = p.filename().string(); // File basename (notes.txt)
		if (regex_match(name.c_str(), match, midifile)) {
			 s.midifilename = name;
		} else if (regex_match(name.c_str(), match, audiofile_background)) {
			testAndAdd(s, "background", name);
		} else if (regex_match(name.c_str(), match, audiofile_guitar)) {
			testAndAdd(s, TrackName::GUITAR, name);
		} else if (regex_match(name.c_str(), match, audiofile_bass)) {
			testAndAdd(s, TrackName::BASS, name);
		} else if (regex_match(name.c_str(), match, audiofile_keyboard)) {
			testAndAdd(s, TrackName::KEYBOARD, name);
		} else if (regex_match(name.c_str(), match, audiofile_drums)) {
			testAndAdd(s, TrackName::DRUMS, name);
		} else if (regex_match(name.c_str(), match, audiofile_vocals)) {
			testAndAdd(s, "vocals", name);
#if 0  // TODO: process preview.ogg properly? In any case, do not print debug to console...
		} else if (regex_match(name.c_str(), match, audiofile_other)) {
			std::cout << "Found unknown ogg file: " << name << std::endl;
#endif
		}
	}
*/

}

void SongParser::resetNoteParsingState() {
	m_prevtime = 0;
	m_prevts = 0;
	m_relativeShift = 0;
	m_tsPerBeat = 0;
	m_tsEnd = 0;
	m_bpms.clear();
	if (m_bpm != 0.0) addBPM(0, m_bpm);
}

void SongParser::vocalsTogether() {
	auto togetherIt = m_song.vocalTracks.find("Together");
	if (togetherIt == m_song.vocalTracks.end()) return;
	Notes& together = togetherIt->second.notes;
	if (!together.empty()) return;
	Notes notes;
	// Collect usable vocal tracks
	struct TrackInfo {
		typedef Notes::const_iterator It;
		It it, end;
		TrackInfo(It begin, It end): it(begin), end(end) {}
	};
	std::vector<TrackInfo> tracks;
	for (auto& nt: m_song.vocalTracks) {
		Notes& n = nt.second.notes;
		if (!n.empty()) tracks.push_back(TrackInfo(n.begin(), n.end()));
	}
	if (tracks.empty()) return;
	// Combine notes
	// FIXME: This should do combining on sentence level rather than note-by-note
	TrackInfo* trk = &tracks.front();
	while (trk) {
		Note const& n = *trk->it;
		//std::cerr << " " << n.syllable << ": " << n.begin << "-" << n.end << std::endl;
		notes.push_back(n);
		++trk->it;
		trk = nullptr;
		// Iterate all tracks past the processed note and find the new earliest track
		for (TrackInfo& trk2: tracks) {
			// Skip until a sentence that begins after the note ended
			while (trk2.it != trk2.end && trk2.it->begin < n.end) ++trk2.it;
			if (trk2.it == trk2.end) continue;
			if (!trk || trk2.it->begin < trk->it->begin) trk = &trk2;
		}
	}
	together.swap(notes);
}

void SongParser::finalize() {
	vocalsTogether();
	for (auto& nt: m_song.vocalTracks) {
		VocalTrack& vocal = nt.second;
		// Remove empty sentences
		{
			Note::Type lastType = Note::NORMAL;
			for (auto itn = vocal.notes.begin(); itn != vocal.notes.end();) {
				Note::Type type = itn->type;
				if(type == Note::SLEEP && lastType == Note::SLEEP) {
					std::clog << "songparser/warning: Discarding empty sentence" << std::endl;
					itn = vocal.notes.erase(itn);
				} else {
					++itn;
				}
				lastType = type;
			}
		}
		// Adjust negative notes
		if (vocal.noteMin <= 0) {
			unsigned int shift = (1 - vocal.noteMin / 12) * 12;
			vocal.noteMin += shift;
			vocal.noteMax += shift;
			for (auto& elem: vocal.notes) {
				elem.note += shift;
				elem.notePrev += shift;
			}
		}
		// Set begin/end times
		if (!vocal.notes.empty()) vocal.beginTime = vocal.notes.front().begin, vocal.endTime = vocal.notes.back().end;
		else vocal.beginTime = vocal.endTime = 0.0;
		// Compute maximum score
		double max_score = 0.0;
		for (auto& note: vocal.notes) max_score += note.maxScore();
		vocal.m_scoreFactor = 1.0 / max_score;
	}
	if (m_tsPerBeat) {
		// Add song beat markers
		for (unsigned ts = 0; ts < m_tsEnd; ts += m_tsPerBeat) m_song.beats.push_back(tsTime(ts));
	}
}
