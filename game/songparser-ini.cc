#include "songparser.hh"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include "midifile.hh"

/// @file
/// Functions used for parsing the UltraStar TXT song format

bool SongParser::iniCheck(std::vector<char> const& data) {
	static const std::string header = "[song]";
	return std::equal(header.begin(), header.end(), data.begin());
}

namespace {
	void eraseLast(std::string& s, char ch = ' ') {
		if (!s.empty() && *s.rbegin() == ch) s.erase(s.size() - 1);
	}
	void testAndAdd(Song& s, std::string const& trackid, std::string const& filename) {
		std::string f = s.path + filename;
		if (boost::filesystem::exists(f)) s.music[trackid] = f;
	}
}

void SongParser::iniParse() {
	Song& s = m_song;
	std::string line;
	while (getline(line)) {
		if (line.empty()) continue;
		if (line[0] == '[') continue; // Section header
		std::istringstream iss(line);
		std::string key, value;
		if (!std::getline(iss, key, '=') || !std::getline(iss, value)) std::runtime_error("Invalid format, should be key=value");
		boost::trim(key); boost::to_lower(key);
		boost::trim(value);
		if (key == "name") s.title = value;
		else if (key == "artist") s.artist = value;
	}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");

	boost::regex midifile("(.*\\.mid)$", boost::regex_constants::icase);
	boost::regex audiofile_background("(song\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_guitar("(guitar\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_drums("(drums\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_bass("(rhythm\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_vocals("(vocals\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_other("(.*\\.ogg)$", boost::regex_constants::icase);
	boost::cmatch match;
	std::string midifilename("notes.mid");

	for (boost::filesystem::directory_iterator dirIt(s.path), dirEnd; dirIt != dirEnd; ++dirIt) {
		boost::filesystem::path p = dirIt->path();
		std::string name = p.leaf(); // File basename (notes.txt)
		if (regex_match(name.c_str(), match, midifile)) {
			 midifilename = name;
		} else if (regex_match(name.c_str(), match, audiofile_background)) {
			testAndAdd(s, "background", name);
		} else if (regex_match(name.c_str(), match, audiofile_guitar)) {
			testAndAdd(s, "guitar", name);
		} else if (regex_match(name.c_str(), match, audiofile_bass)) {
			testAndAdd(s, "bass", name);
		} else if (regex_match(name.c_str(), match, audiofile_drums)) {
			testAndAdd(s, "drums", name);
		} else if (regex_match(name.c_str(), match, audiofile_vocals)) {
			testAndAdd(s, "vocals", name);
		} else if (regex_match(name.c_str(), match, audiofile_other)) {
			std::cout << "Found unknown ogg file: " << name << std::endl;
		}
	}

	MidiFileParser midi(s.path + "/" + midifilename);
	for (uint32_t ts = 0, end = midi.ts_last + midi.division; ts < end; ts += midi.division) s.beats.push_back(midi.get_seconds(ts));
	for (MidiFileParser::Tracks::const_iterator it = midi.tracks.begin(); it != midi.tracks.end(); ++it) {
		// Figure out the track name
		std::string name = it->name;
		if (midi.tracks.size() == 1) name = "GUITAR"; // Original (old) FoF songs only have one track
		else if (name == "T1 GEMS") name = "GUITAR"; // Some old MIDI files have a track named T1 GEMS
		else if (name.substr(0, 5) != "PART ") continue;
		else name.erase(0, 5);
		if (name == "GUITAR COOP") continue; // TODO: do something with these? They don't work in current version
		if (name == "DRUM") name = "DRUMS";
		// Process non-vocal tracks
		if (name != "VOCALS") {
			bool drums = (name == "DRUMS");
			s.tracks.push_back(Track(name));
			NoteMap& nm = s.tracks.back().nm;
			for (MidiFileParser::NoteMap::const_iterator it2 = it->notes.begin(); it2 != it->notes.end(); ++it2) {
				Durations& dur = nm[it2->first];
				MidiFileParser::Notes const& notes = it2->second;
				for (MidiFileParser::Notes::const_iterator it3 = notes.begin(); it3 != notes.end(); ++it3) {
					double beg = midi.get_seconds(it3->begin);
					double end = midi.get_seconds(it3->end);
					if (end == 0) continue; // Note with no ending
					if (beg > end) throw std::runtime_error("Reversed notes");
					if (drums) end = beg;
					dur.push_back(Duration(beg, end));
				}
			}
			continue;
		}
		// Process vocal tracks
		for (MidiFileParser::Lyrics::const_iterator it2 = it->lyrics.begin(); it2 != it->lyrics.end(); ++it2) {
			Note n;
			n.begin = midi.get_seconds(it2->begin);
			n.end = midi.get_seconds(it2->end);
			n.notePrev = n.note = it2->note;
			n.type = n.note > 100 ? Note::SLEEP : Note::NORMAL;
			{
				std::stringstream ss(it2->lyric);
				convertToUTF8(ss);
				n.syllable = ss.str();
			}
			std::string& syl = n.syllable;
			if (n.type != Note::SLEEP) {
				if (!syl.empty()) {
					bool erase = false;
					// Examine note styles (specified by the last character of the syllable)
					{
						char& ch = *syl.rbegin();
						if (ch == '#') { n.type = Note::FREESTYLE; erase = true; }
						if (ch == '^') { n.type = Note::GOLDEN; erase = true; }
						if (ch == '+') { n.type = Note::SLIDE; ch = '~'; }
					}
					if (erase) syl.erase(syl.size() - 1);
					// Add spaces between words, remove hyphens
					{
						char ch = *syl.rbegin();
						if (ch == '-') syl.erase(syl.size() - 1);
						else if (ch != '~') syl += ' ';
					}
				}
				// Special processing for slides (which depend on the previous note)
				if (n.type == Note::SLIDE) {
					Notes::reverse_iterator prev = s.notes.rbegin();
					while (prev != s.notes.rend() && prev->type == Note::SLEEP) ++prev;
					if (prev == s.notes.rend()) throw std::runtime_error("The song begins with a slide note");
					eraseLast(prev->syllable); // Erase the space if there is any
					{
						// insert new sliding note
						Note inter;
						inter.begin = prev->end;
						inter.end = n.begin;
						inter.notePrev = prev->note;
						inter.note = n.note;
						inter.type = Note::SLIDE;
						inter.syllable = std::string("~");
						m_maxScore += inter.maxScore();
						s.noteMin = std::min(s.noteMin, inter.note);
						s.noteMax = std::max(s.noteMax,inter.note);
						s.notes.push_back(inter);
					}
					{
						// modifying current note to be normal again
						n.type = Note::NORMAL;
					}
				}
				m_maxScore += n.maxScore();
				s.noteMin = std::min(s.noteMin, n.note);
				s.noteMax = std::max(s.noteMax, n.note);
				s.notes.push_back(n);
			} else if (!s.notes.empty() && s.notes.back().type != Note::SLEEP) {
				eraseLast(s.notes.back().syllable);
				s.notes.push_back(n);
			}
		}
		if (!s.notes.empty()) break;
	}
	/*if (s.notes.empty()) {
		Note n;
		n.begin = 30.0;
		n.end = 31.0;
		n.syllable = "TODO";
		s.noteMin = s.noteMax = n.note = 60;
		s.notes.push_back(n);
	}*/
}


