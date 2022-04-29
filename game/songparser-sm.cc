#include "songparser.hh"
#include "unicode.hh"
#include "fs.hh"

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <stdexcept>
#include <map>

/// @file
/// Functions used for parsing the StepMania SM format

using namespace SongParserUtil;

/// 'Magick' to check if this file looks like correct format
bool SongParser::smCheck(std::string const& data) const {
	if (data[0] != '#' || data[1] < 'A' || data[1] > 'Z') return false;
	for (char ch: data) {
		if (ch == '\n') return false;
		if (ch == ';') return true;
	}
	return false;
}

/* Parsing the note data is separated into three different functions: smParse, smParseField and smParseNote.
- smParse only begins a loop which continues as long as there is something to read in the file. It also checks if the needed information
could be read.
- smParseField reads all data beginning with '#'. That is, all but the actual notes. This function calls smParseNotes every time it
reaches value #NOTES.
- smParseNotes reads the notes into vector called notes which is a vector of structs (Note);
*/

/// Parse header data for Songs screen
// TODO: This actually parses the whole thing
// TODO: Songparser drops parsed notes, remove it when smParseHeader is more intelligent
void SongParser::smParseHeader() {
	Song& s = m_song;
	std::string line;
	if (!m_song.danceTracks.empty()) { m_song.danceTracks.clear(); }
	// Parse the the entire file
	while (getline(line) && smParseField(line)) {}
	if (m_song.danceTracks.empty() ) throw std::runtime_error("No note data in the file");
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	// Convert stops to the format required in Song
	s.stops.resize(m_stops.size());
	for (std::size_t i = 0; i < m_stops.size(); ++i) s.stops[i] = smStopConvert(m_stops[i]);
	m_tsPerBeat = 4;
}

/// Parse remaining stuff
void SongParser::smParse() {
	m_song.stops.clear();
	m_song.danceTracks.clear();
	smParseHeader();
}

bool SongParser::smParseField(std::string line) {
	boost::trim(line);
	if (line.empty()) return true;
	if (line.substr(0, 2) == "//") return true; //jump over possible comments
	if (line[0] == ';') return true; // HACK: Skip ; left over from previous field

	//Here the data contained by the current line is separated in key and value.
	//However, because of the differing format of notedata the value is analyzed only if key is not NOTES
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid sm format, should be #key:value");
	std::string key = boost::trim_copy(line.substr(1, pos - 1));
	if (key == "NOTES") {
		/*All remaining data is parsed here.
			All five lines of note metadata is read first and then smParseNotes is called to read
			the actual note data.
			All data is read into m_song.danceTracks map container.
		*/

		while (getline(line)) {
			//<NotesType>:
			std::string notestype = boost::trim_copy(line.substr(0, line.find_first_of(':')));
			notestype = UnicodeUtil::toLower(notestype);
			//<Description>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string description = boost::trim_copy(line.substr(0, line.find_first_of(':')));
			//<DifficultyClass>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string difficultyclass = boost::trim_copy(line.substr(0, line.find_first_of(':')));
			difficultyclass = UnicodeUtil::toUpper(difficultyclass);
			DanceDifficulty danceDifficulty = DanceDifficulty::COUNT;
			if(difficultyclass == "BEGINNER") danceDifficulty = DanceDifficulty::BEGINNER;
			if(difficultyclass == "EASY") danceDifficulty = DanceDifficulty::EASY;
			if(difficultyclass == "MEDIUM") danceDifficulty = DanceDifficulty::MEDIUM;
			if(difficultyclass == "HARD") danceDifficulty = DanceDifficulty::HARD;
			if(difficultyclass == "CHALLENGE") danceDifficulty = DanceDifficulty::CHALLENGE;

			//ignoring difficultymeter and radarvalues
			//<DifficultyMeter>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }

			//<NoteData>:
			Notes notes = smParseNotes(line);

			//Here all note data from the current track is inserted into containers
			// TODO: support other track types. For now all others are simply ignored.
			if (notestype == "dance-single" || notestype == "dance-double" || notestype == "dance-solo"
			  || notestype == "pump-single" || notestype == "ez2-single" || notestype == "ez2-real"
			  || notestype == "para-single") {
				DanceTrack danceTrack(description, notes);
				if (m_song.danceTracks.find(notestype) == m_song.danceTracks.end() ) {
					DanceDifficultyMap danceDifficultyMap;
					m_song.danceTracks.insert(std::make_pair(notestype, danceDifficultyMap));
				}
				m_song.danceTracks[notestype].insert(std::make_pair(danceDifficulty, danceTrack));
			}
		}
		return false;
	}
	std::string value = boost::trim_copy(line.substr(pos + 1));
	//In case the value continues to several lines, all text before the ending character ';' is read to single line.
	while (value[value.size() -1] != ';') {
		std::string str;
		if (!getline (str)) throw std::runtime_error("Invalid format, semicolon missing after value of " + key);
		value += boost::trim_copy(str);
	}
	value = value.substr(0, value.size() - 1);	//Here the end character(';') is eliminated
	if (value.empty()) return true;

	// Parse header data that is stored in SongParser rather than in song (and thus needs to be read every time)
	if (key == "OFFSET") { assign(m_gap, value); m_gap *= -1; }
	else if (key == "BPMS"){
			std::istringstream iss(value);
			float ts, bpm;
			char chr;
			while (iss >> ts >> chr >> bpm) {
				if (ts == 0.0) m_bpm = bpm;
				addBPM(ts * 4.0f, bpm);
				if (!(iss >> chr)) break;
			}
	}
	else if (key == "STOPS"){
			std::istringstream iss(value);
			float beat, sec;
			char chr;
			while (iss >> beat >> chr >> sec) {
				m_stops.push_back(std::make_pair(beat * 4.0, sec));
				if (!(iss >> chr)) break;
			}
	}

	if (m_song.loadStatus >= Song::LoadStatus::HEADER) return true;  // Only re-parsing now, skip any other data

	// Parse header data that is directly stored in m_song
	if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
	else if (key == "BANNER") m_song.cover = absolute(value, m_song.path);
	else if (key == "MUSIC") m_song.music[TrackName::BGMUSIC] = absolute(value, m_song.path);
	else if (key == "BACKGROUND") m_song.background = absolute(value, m_song.path);
	else if (key == "SAMPLESTART") assign(m_song.preview_start, value);
	/*.sm fileformat has also the following constants but they are ignored in this version of the parser:
	#SUBTITLE
	#TITLETRANSLIT
	#SUBTITLETRANSLIT
	#ARTISTTRANSLIT
	#CREDIT
	#CDTITLE
	#SAMPLESTART
		#SAMPLELENGTH
	#SELECTABLE
	#BGCHANGE
	*/
	// Add beat info to song
	return true;
}




Notes SongParser::smParseNotes(std::string line) {
	//container for dance songs
	typedef std::map<int, Note> DanceChord;	//int indicates "arrow" position (cmp. fret in guitar)
	typedef std::vector<DanceChord> DanceChords;

	DanceChords chords;	//temporary container for notes
	Notes notes;
	unsigned measure = 1;
	float begin = 0.0f;
	bool forceMeasure = false;

	std::map<int, int> holdMarks; // Keeps track of hold notes not yet terminated

	while (forceMeasure || getline(line)) {
		if (forceMeasure) { line = ";"; forceMeasure = false; }
		boost::trim(line); // Remove whitespace
		if (line.empty()) continue;
		if (line.substr(0, 2) == "//") continue;  // Skip comments
		if (line[0] == '#') break;  // HACK: This should read away the next #NOTES: line
		if (line[0] == ',' || line[0] == ';') {
			float end = tsTime(measure * 16.0f);
			unsigned div = chords.size();
			float step = (end - begin) / div;
			for (unsigned note = 0; note < div; ++note) {
				float t = begin + note * step;
				float phase = float(note) / div;
				for (auto& elem: chords[note]) {
					int& holdIdx = holdMarks[elem.first];  // holdIdx for current arrow
					Note& n = elem.second;
					n.begin = n.end = t;
					n.phase = phase;
					// TODO: Proper LIFT handling
					if (n.type == Note::Type::TAP || n.type == Note::Type::MINE || n.type == Note::Type::LIFT) notes.push_back(n);
					// TODO: Proper ROLL handling
					if (n.type == Note::Type::HOLDBEGIN || n.type == Note::Type::ROLL) {
						notes.push_back(n);  // Note added now, end time will be fixed later
						holdIdx = notes.size(); // Store index in notes plus one
						continue;
					}
					if (n.type == Note::Type::HOLDEND) {
						if (holdIdx == 0) throw std::runtime_error("Hold end without beginning");
						notes[holdIdx - 1].end = t;
					}
					holdIdx = 0;
				}
			}
			chords.clear();
			begin = end;
			++measure;
			continue;
		}
		/*Note data is read into temporary container chords before
		finally reading it into vector notes. This is done so that the bpm and time stamp values
		would be easier to count afterwards.
		*/
		DanceChord chord;
		// Deal with ; or , being on a same line
		if (line[line.size()-1] == ';' || line[line.size()-1] == ',') {
			forceMeasure = true;
			line = line.substr(0, line.size()-1);
		}
		for(std::size_t i = 0; i < line.size(); i++) {
			char notetype = line[i];
			if (notetype == '0') continue;
			Note note;
			if(notetype == '1') note.type = Note::Type::TAP;
			else if(notetype == '2') note.type = Note::Type::HOLDBEGIN;
			else if(notetype == '3') note.type = Note::Type::HOLDEND;
			else if(notetype == '4') note.type = Note::Type::ROLL;
			else if(notetype == 'M') note.type = Note::Type::MINE;
			else if(notetype == 'L') note.type = Note::Type::LIFT;
			else if(notetype >= 'a' && notetype <= 'z') note.type = Note::Type::TAP;
			else if(notetype >= 'A' && notetype <= 'Z') note.type = Note::Type::TAP;
			else throw std::runtime_error(std::string("Invalid note command: ") + notetype);
			note.note = i;
			chord[i] = note;
		}
		chords.push_back(chord);
	}
	m_tsEnd = std::max(m_tsEnd, measure * 16);
	return notes;
}

/// Convert a stop into <time, duration> (as stored in the song)
std::pair<float, float> SongParser::smStopConvert(std::pair<float, float> s) {
	s.first = tsTime(s.first);
	return s;
}
