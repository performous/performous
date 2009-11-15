#include "songparser.hh"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <stdexcept>
#include <map>

/// @file
/// Functions used for parsing the StepMania SM format


namespace {
	bool isNote(const char& c) {
		return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == 'M' || c == 'L';
	}

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
	int lcount = 0;		//line counter for note duration
	int count = 0; 		//total lines counter
	bool readable = false; 	//makes sure that other values are read before notes
	double tm = 0; 		//time counter
	double dur; 		//note duration
	std::string notestype;
	std::string description;
	DanceDifficulty danceDifficulty = DIFFICULTYCOUNT;
	DanceDifficultyMap danceDifficultyMap;
	DanceChords chords;

	while (getline(line)) {
	
		if (line.empty() || line == "\r") continue;
		if (line[0] == '/' && line[1] == '/') continue; //jump over possible comments
		//reading of the values
		if (readable && isNote(line[0]) ) {
			//reading notes into a chord 
			std::istringstream iss(line);
			DanceChord chord;
			for(int i =1; i <= 4; i++){
				if(iss.get() == '1') {
					Note note;	
					chord[i] = note;
				}
			//if (iss.get() == ';') return false;	//for the case where end mark ';' is not on its own line
				//TODO hold etc. notes
			chords.push_back(chord);
			lcount++;
			count++;
			}
			continue;
		}	
		if (!readable && isNote(line[0]) ) throw std::runtime_error("Notes outside NoteData");
		if (line[0] == ',') {
			/*Counting the timestamp of each note*/
			dur = 4.0 * ( 1.0/(m_bpm/60.0) ) / lcount;	//counts one note duration in seconds;	
			//std::cout << "DUR: " << dur << "TM: " << tm << std::endl;
			for(int j = count - lcount; j<count ; j++) {	
				for(int i = 0; i<4; i++) {
					DanceChord& _chord = chords.at(j);
					if(_chord.find(i) != _chord.end()) {
						_chord[i].begin = tm;
						_chord[i].end = tm;
					}				
				}
				tm += dur;
			}
			lcount =0;
			continue;
		}
		if (line[0] == ';') continue;
	
		std::string::size_type pos = line.find(':');
		if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key:value");
		std::string key = boost::trim_copy(line.substr(1, pos - 1));
		if (key == "NOTES") {	
			//if not the first time
			if(readable) {
				DanceTrack danceTrack(description, chords);
				danceDifficultyMap.insert(std::make_pair(danceDifficulty, danceTrack));
				m_song.danceTracks.insert(std::make_pair(notestype, danceDifficultyMap));
			}
			//temporary containers for note data
			/*DanceDifficultyMap danceDifficultyMap;
			DanceChords chords;*/
		
			//Note values are analyzed here. Values are:
		
			// TODO: NOTES header vars needs better way to strip trailing colon ':'
			
			//<NotesType>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			notestype = boost::trim_copy(line.substr(0, line.size() -2));
			//<Description>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			description = boost::trim_copy(line.substr(0, line.size() -2));
			//<DifficultyClass>:
			if(!getline(line)) { throw std::runtime_error("Required note data missing"); }
			std::string difficultyclass = boost::trim_copy(line.substr(0, line.size() -2));
			//enum			
				transform(difficultyclass.begin(), difficultyclass.end(), 
				  difficultyclass.begin(), upper_case );
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
			/*while (getline(line) && smParseNotes(line, chords)) {}
			
			DanceTrack danceTrack(description, chords);
			danceDifficultyMap.insert(std::make_pair(danceDifficulty, danceTrack));
			m_song.danceTracks.insert(std::make_pair(notestype, danceDifficultyMap));*/	
			
			readable = true;
			continue;
		}
		std::string value = boost::trim_copy(line.substr(pos + 1)); 
		value.erase(value.size() -1);	// compared to txtParseField, here last character(';') is eliminated
		if (value.empty()) continue;
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
			while(std::getline(iss, ts, '=') && std::getline(iss, bpm, ',')) {
				if(atof(ts.c_str()) == 0) assign(m_bpm, bpm);
				else addBPM(atof(ts.c_str()), atof(bpm.c_str()));
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
		continue;
		}
	if (m_song.danceTracks.empty() ) throw std::runtime_error("No note data in the file");
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	if (m_bpm != 0.0) addBPM(0, m_bpm);
	
}											
