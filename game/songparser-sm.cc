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
}



bool SongParser::smCheck(std::vector<char> const& data) { return data[0] == '#' && data[1] >= 'A' && data[1] <= 'Z' && data.back() == ';'; }

void SongParser::smParse() {
	Song& s = m_song;
	std::string line;
	while (getline(line) && smParseField(line)) {}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	if (m_bpm != 0.0) addBPM(0, m_bpm);
	
	// Workaround for the terminating : 1 0 0 line, written by some converters
/*	if (!s.notes.empty() && s.notes.back().type != Note::SLEEP && s.notes.back().begin == s.notes.back().end) s.notes.pop_back();
*/
}

bool SongParser::smParseField(std::string line) {
	if (line.empty()) return true;
	if (line[0] == '/' && line[1] == '/') return true; //jump over possible comments
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key:value");
	std::string key = boost::trim_copy(line.substr(1, pos - 1));
	if (key == "NOTES") {	

	//temporary containers for note data
	DanceTrackMap danceTrackMap;
	DanceChords chords;
	
	//Note values are analyzed here. Values are:

	//<NotesType>:
	if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
	std::string notestype = boost::trim_copy(line.substr(0, line.size() -1);
	//<Description>:
	if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
	std::string description = boost::trim_copy(line.substr(0, line.size() -1));
	m_song.dance_data.addDescription(description);
	//<DifficultyClass>:
	if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
	std::string difficultyclass = boost::trim_copy(line.substr(0, line.size() -1));
	//enum	
	DanceDifficulty danceDifficulty;		
	if(difficultyclass == "BEGINNER") danceDifficulty = BEGINNER;
	if(difficultyclass == "EASY") danceDifficulty = EASY;
	if(difficultyclass == "MEDIUM") danceDifficulty = MEDIUM;
	if(difficultyclass == "HARD") danceDifficulty = HARD;
	if(difficultyclass == "CHALLENGE") danceDifficulty = CHALLENGE;

	//ignoring difficultymeter and radarvalues
	//<DifficultyMeter>:
	if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
	//std::string difficultymeter = boost::trim_copy(line.substr(0, line.size() -1));
	//<RadarValues>:
	if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
	//std::string radarvalues = boost::trim_copy(line.substr(0, line.size() -1));
	
	//<NoteData>:
	while (getline(line) && smParseNote(line, chords)) {}
	
	DanceTrack danceTrack(description, chords);
	danceTrackMap[danceDifficulty] = danceTrack;
	m_song.danceTracks[notestype] = danceTrackMap;	
	return true;
	}

	std::string value = boost::trim_copy(line.substr(pos + 1)); 
	value.erase(value.size() -1);	// compared to txtParseField, here last character(';') is eliminated
	if (value.empty()) return true;
	if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
	else if (key == "BANNER") m_song.cover = value;
	else if (key == "MUSIC") m_song.music["background"] = m_song.path + value;
	else if (key == "BACKGROUND") m_song.background = value;
	else if (key == "OFFSET") assign(m_song.start, value);
	else if (key == "BPMS"){
	std::replace(value.begin(), value.end(), ';', ','); // replacing the last ';'
	std::istringstream iss(value);
	std::string ts, bpm; //ts is the timestamp of the bpm
	while(std::getline(iss, ts, '=') && std::getline(iss, bpm, ','))
		if(atof(ts.c_str()) == 0) assign(m_bpm, bpm);
		else addBPM(atof(ts.c_str()), atof(bpm.c_str()));
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

bool SongParser::smParseNote(std::string line, DanceChords chords){
	if(line.empty()) return true;
	if(line[0] == '#') throw std::runtime_error("Key found in the middle of notes");
	if(line[0] == ';') return false; //end mark	

	//reading the notes into m_song.dance_data
	int lcount = 0;	//line counter for bpm
	int count = 0; //total lines counter
	int tm = 0; 	//time counter
	int dur; 	//note duration
	if(line[0] == ',') {
		/*Counting the timestamp of each note*/
		dur = 4 * ( 1/(m_bpm/60) ) / lcount;	//counts one note duration in seconds;	
		for(int j = count - lcount; j<count ; j++) {	
			for(int i = 0; i<4; i++) {
				if(chords[j][i]) chords[j][i].begin = tm;
				if(chords[j][i]) chords[j][i].end = tm;		
				
			}
			tm += dur;
		}
		lcount =0;
		return true;
	}

	//reading notes into a chord 
	std::istringstream iss(line);
	DanceChord chord;
	for(int i =1; i <= 4; i++){
		if(iss.get() == '1') {
			Note note;	
			chord[i] = note;
		}
		//TODO hold etc. notes
	chords.push_back(chord);
	lcount++;
	}
	return true;
}
