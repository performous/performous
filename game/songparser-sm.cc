#include "songparser.hh"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <stdexcept>
#include <map>

/// @file
/// Functions used for parsing the StepMania SM format


namespace {
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
}



bool SongParser::smCheck(std::vector<char> const& data) {
	if (data[0] != '#' || data[1] < 'A' || data[1] > 'Z') return false;
	for (std::vector<char>::const_iterator it = data.begin(); it != data.end(); ++it)
		if (*it == ';') return true;
	return false;
}

void SongParser::smParse() {
	Song& s = m_song;
	std::string line;
	while (getline(line) && smParseField(line)) {}
	if (m_song.danceTracks.empty() ) throw std::runtime_error("No note data in the file");
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	//if (m_bpm != 0.0) addBPM(0, m_bpm);
}	
bool SongParser::smParseField(std::string line) {
		if (line.empty() || line == "\r") return true;
		if (line[0] == '/' && line[1] == '/') return true; //jump over possible comments
		if (line[0] == ';') return true;
		std::string::size_type pos = line.find(':');
		if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key:value");
		std::string key = boost::trim_copy(line.substr(1, pos - 1));
		if (key == "NOTES") {
		
			//Note values are analyzed here. Values are:
		bool endOfInput = false;
			// TODO: NOTES header vars needs better way to strip trailing colon ':'
			while (getline(line)) {
			//<NotesType>:
			//if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string notestype = boost::trim_copy(line.substr(0, line.size() -2));
			//<Description>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string description = boost::trim_copy(line.substr(0, line.size() -2));
			//<DifficultyClass>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string difficultyclass = boost::trim_copy(line.substr(0, line.size() -2));
			transform(difficultyclass.begin(), difficultyclass.end(), 
			difficultyclass.begin(), upper_case );
			DanceDifficulty danceDifficulty = DIFFICULTYCOUNT;
				if(difficultyclass == "BEGINNER") danceDifficulty = BEGINNER;
				if(difficultyclass == "EASY") danceDifficulty = EASY;
				if(difficultyclass == "MEDIUM") danceDifficulty = MEDIUM;
				if(difficultyclass == "HARD") danceDifficulty = HARD;
				if(difficultyclass == "CHALLENGE") danceDifficulty = CHALLENGE;

			//ignoring difficultymeter and radarvalues
			//<DifficultyMeter>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			//std::string difficultymeter = boost::trim_copy(line.substr(0, line.size() -2));
			//<RadarValues>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			//std::string radarvalues = boost::trim_copy(line.substr(0, line.size() -2));
			
			//<NoteData>:
			Notes notes = smParseNotes(line, endOfInput);
			if(endOfInput) throw std::runtime_error("end is here");
			
			
			DanceTrack danceTrack(description, notes);
			DanceDifficultyMap danceDifficultyMap;
			danceDifficultyMap.insert(std::make_pair(danceDifficulty, danceTrack));
			m_song.danceTracks.insert(std::make_pair(notestype, danceDifficultyMap));
			}
			return false;
		}
		std::string value = boost::trim_copy(line.substr(pos + 1));
		//values that continue to next line are handeled here
		while (value[value.size() -1] != ';') {			
			std::string str;			
			getline (str);
			value += boost::trim_copy(str);
		}
		value = value.substr(0, value.size() - 1);	// compared to txtParseField, here last character(';') is eliminated
		if (value.empty()) return true;
		if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
		else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
		else if (key == "BANNER") m_song.cover = value;
		else if (key == "MUSIC") m_song.music["background"] = m_song.path + value;
		else if (key == "BACKGROUND") m_song.background = value;
		else if (key == "OFFSET") assign(m_song.start, value);
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
		/*additionally .sm fileformat has following constants:
		#SUBTITLE
		#TITLETRANSLIT
		#SUBTITLETRANSLIT
   		#ARTISTTRANSLIT
		#CREDIT
   		#CDTITLE
		#SAMPLESTART
    		#SAMPLELENGTH
		#SELECTABLE
    		#STOPS
		#BGCHANGE
		*/
		return true;
}

Notes SongParser::smParseNotes(std::string line, bool endOfInput) {
	DanceChords chords;	//temporary container for notes
	
	Notes notes;	
	int lcount = 0;		//line counter for note duration
	int count = 0; 		//total lines counter
	double tm = 0; 		//time counter
	double dur; 		//note duration

	std::vector<int> holdMarks(4, -1);	//vector to contain mark for the chord where hold began for each "fret" (-1 if no mark set)

	while(getline(line)) {
		if (line.empty() || line == "\r") continue;
		if (line[0] == '/' && line[1] == '/') continue;
		if (line[0] == '#') return notes;
		if (line[0] == ';') continue;
		if (line[0] == ',') {
			/*Counting the timestamp of each note*/
			dur = 4.0 * ( 1.0/(m_bpm/60.0) ) / lcount;	//counts one note duration in seconds;	
			//std::cout << "DUR: " << dur << "TM: " << tm << std::endl;
			for(int j = count - lcount; j<count ; j++) {
				DanceChord& _chord = chords.at(j);
				for(int i = 0; i<4; i++) {
					if(_chord.find(i) != _chord.end()) {
						if(_chord[i].type == Note::TAP || _chord[i].type == Note::MINE) {
						_chord[i].begin = tm;
						_chord[i].end = tm;
						notes.push_back(_chord[i]);	//note added to notes container used in DanceTrack
						}
						if(_chord[i].type == Note::HOLDBEGIN) {
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
		//reading notes into a temporary container chord
		DanceChord chord;
		std::istringstream iss(line);
		for(int i =0; i<4; i++){
			char notetype = iss.get();
			if(notetype == '1') {
				Note note;	
				note.note = i;
				note.type = Note::TAP;
				chord[i] = note;
			}
			if(notetype == 'M') {
				Note note;	
				note.note = i;
				note.type = Note::MINE;
				chord[i] = note;
			}
			
			if(notetype == '2') {
				Note note;
				note.note = i;
				note.type = Note::HOLDBEGIN;
				chord[i] = note;
			}
			if(notetype == '3') {
				Note note;
				note.note = i;
				note.type = Note::HOLDEND;
				chord[i] = note;
			}
		}
		lcount++;
		count++;
		chords.push_back(chord);
		continue;
	}
	endOfInput = true;
	return notes;
}									
