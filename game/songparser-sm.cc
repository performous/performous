#include "songparser.hh"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <stdexcept>
#include <map>

/// @file
/// Functions used for parsing the StepMania SM format


namespace {
	
	const int max_panels = 10; // Maximum number of arrow lines
	
	// Here are some functions needed in reading the data.
	void assign(int& var, std::string const& str) {
		try {
			var = boost::lexical_cast<int>(str);
		} catch (...) {
			throw std::runtime_error("\"" + str + "\" is not valid integer value");
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
	int upper_case (int c) { return toupper (c); }
	int lower_case (int c) { return tolower (c); }
}



bool SongParser::smCheck(std::vector<char> const& data) {
	if (data[0] != '#' || data[1] < 'A' || data[1] > 'Z') return false;
	for (std::vector<char>::const_iterator it = data.begin(); it != data.end(); ++it)
		if (*it == ';') return true;
	return false;
}

/* Parsing the note data is separated into three different functions: smParse, smParseField and smParseNote.
- smParse only begins a loop which continues as long as there is something to read in the file. It also checks if the needed information
could be read.
- smParseField reads all data beginning with '#'. That is, all but the actual notes. This function calls smParseNotes every time it
reaches value #NOTES.
- smParseNotes reads the notes into vector called notes which is a vector of structs (Note);
*/

void SongParser::smParse() {
	Song& s = m_song;
	std::string line;
	while (getline(line) && smParseField(line)) {}
	if (m_song.danceTracks.empty() ) throw std::runtime_error("No note data in the file");
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	std::string& music = s.music["background"];
	std::string tmp = s.path + "music.ogg";
	namespace fs = boost::filesystem;
	if ((music.empty() || !fs::exists(music)) && fs::exists(tmp)) music = tmp;
}
	
bool SongParser::smParseField(std::string line) {
	if (line.empty() || line == "\r") return true;
	if (line[0] == '/' && line[1] == '/') return true; //jump over possible comments
	if (line[0] == ';') return true;

	//Here the data contained by the current line is separated in key and value.
	//However, because of the differing format of notedata the value is analyzed only if key is not NOTES
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key:value");
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
			transform(notestype.begin(), notestype.end(), notestype.begin(), lower_case );
		//<Description>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string description = boost::trim_copy(line.substr(0, line.find_first_of(':')));
		//<DifficultyClass>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string difficultyclass = boost::trim_copy(line.substr(0, line.find_first_of(':')));
			transform(difficultyclass.begin(), difficultyclass.end(), difficultyclass.begin(), upper_case );
			DanceDifficulty danceDifficulty = DIFFICULTYCOUNT;
				if(difficultyclass == "BEGINNER") danceDifficulty = BEGINNER;
				if(difficultyclass == "EASY") danceDifficulty = EASY;
				if(difficultyclass == "MEDIUM") danceDifficulty = MEDIUM;
				if(difficultyclass == "HARD") danceDifficulty = HARD;
				if(difficultyclass == "CHALLENGE") danceDifficulty = CHALLENGE;

		//ignoring difficultymeter and radarvalues
			//<DifficultyMeter>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			
		//<NoteData>:
			Notes notes = smParseNotes(line);

		//Here all note data from the current track is inserted into containers 
			DanceTrack danceTrack(description, notes);
			if (m_song.danceTracks.find(notestype) == m_song.danceTracks.end() ) {
				DanceDifficultyMap danceDifficultyMap;
				m_song.danceTracks.insert(std::make_pair(notestype, danceDifficultyMap));
			}
			m_song.danceTracks[notestype].insert(std::make_pair(danceDifficulty, danceTrack));
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
	if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
	else if (key == "BANNER") m_song.cover = value;
	else if (key == "MUSIC") m_song.music["background"] = m_song.path + value;
	else if (key == "BACKGROUND") m_song.background = value;
	else if (key == "OFFSET") assign(m_song.start, value);
	//Bpm values are only read into a container in class SongParser, but not used (except the first value).
	else if (key == "BPMS"){
			std::istringstream iss(value);
			double ts, bpm;	
			char chr;
			while (iss >> ts >> chr >> bpm) {
				if (ts == 0.0) m_bpm = bpm;
				addBPM(ts, bpm);
				if (!(iss >> chr)) break;
			}
	}
	//Stops are only read into a container in class SongParser, but not used.
	else if (key == "STOPS"){
			std::istringstream iss(value);
			double beat, sec;
			char chr;
			while (iss >> beat >> chr >> sec) {
				m_stops.push_back(std::make_pair(beat, sec));
				if (!(iss >> chr)) break;
	
			}
	}
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
	return true;
}




Notes SongParser::smParseNotes(std::string line) {
	DanceChords chords;	//temporary container for notes
	Notes notes;	
	int lcount = 0;		//line counter for note duration
	int count = 0; 		//total lines counter
	double tm = 0; 		//time counter
	double dur; 		//note duration

	std::vector<int> holdMarks(max_panels, -1);	//vector to contain mark for the chord where hold began for each "fret" (-1 if no mark set)

	while(getline(line)) {
		if (line.empty() || line == "\r") continue;
		if (line[0] == '/' && line[1] == '/') continue;
		if (line[0] == '#') return notes;
		if (line[0] == ',' || line[0] == ';') {
			/*Counting of the time stamps of the notes is done
			every time there is a character ',' which indicates the end of a measure.
			*/
			dur = 4.0 * ( 1.0/(m_bpm/60.0) ) / lcount;	//counts one note duration in seconds;	
			for(int j = count - lcount; j<count ; j++) {
				DanceChord& _chord = chords.at(j);
				for(int i = 0; i < max_panels; i++) {
					if(_chord.find(i) != _chord.end()) {
						// TODO: Proper LIFT handling
						if(_chord[i].type == Note::TAP || _chord[i].type == Note::MINE || _chord[i].type == Note::LIFT) {
							_chord[i].begin = tm;
							_chord[i].end = tm;
							notes.push_back(_chord[i]);	//note added to notes container used in DanceTrack
						}
						// TODO: Proper ROLL handling
						if(_chord[i].type == Note::HOLDBEGIN || _chord[i].type == Note::ROLL) {
							_chord[i].begin = tm;
							notes.push_back(_chord[i]);	//note added to notes container used in DanceTrack
							holdMarks.at(i) = notes.size() - 1; //mark to hold beginning in notes vector
						}
						if(_chord[i].type == Note::HOLDEND) {
							if(holdMarks.at(i) < 0) throw std::runtime_error("hold end without beginning");
							Note& _note = notes.at(holdMarks.at(i));
							_note.end = tm;
							holdMarks.at(i) = -1;
						}
					}
				}
				tm += dur;
			}
			lcount =0;
			continue;
		}
		/*Note data is read into temporary container chords before
		finally reading it into vector notes. This is done so that the bpm and time stamp values
		would be easier to count afterwards.
		*/
		DanceChord chord;
		std::istringstream iss(line);
		for(int i = 0; i < max_panels; i++) {
			if ((unsigned)i > line.size()) break; // end of line reached
			char notetype = iss.get();
			if (notetype == '0') continue;
			Note note;
			if(notetype == '1') note.type = Note::TAP;
			else if(notetype == '2') note.type = Note::HOLDBEGIN;
			else if(notetype == '3') note.type = Note::HOLDEND;
			else if(notetype == '4') note.type = Note::ROLL;
			else if(notetype == 'M') note.type = Note::MINE;
			else if(notetype == 'L') note.type = Note::LIFT;
			else if(notetype >= 'a' && notetype <= 'z') note.type = Note::TAP;
			else if(notetype >= 'A' && notetype <= 'Z') note.type = Note::TAP;
			else continue;
			note.note = i;
			chord[i] = note;
		}
		lcount++;	//lcount and count values will be used to count the time stamps
		count++;
		chords.push_back(chord);
		continue;
	}
	//The code reaches here only when all data is read from the file.
	return notes;
}
