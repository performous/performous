#include "songparser.hh"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <stdexcept>
#include <map>

/// @file
/// Functions used for parsing the StepMania SM format

//needs additions to songparser.hh: smCheck, smParseNote, smParseField
//needs additions to song.hh: typedef std::vector<std::map<int, Note> > d_notes or std::vector<std::vector<Note> > d_notes or similar

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
	while (getline(line) && smParseNote(line)) {}
	// Workaround for the terminating : 1 0 0 line, written by some converters
/*	if (!s.notes.empty() && s.notes.back().type != Note::SLEEP && s.notes.back().begin == s.notes.back().end) s.notes.pop_back();
*/
}

bool SongParser::smParseField(std::string& line) {
	if (line.empty()) return true;
	if (line[0] == '/' && line[1] == '/') return true; //jump over possible comments
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key:value");
	std::string key = boost::trim_copy(line.substr(1, pos - 1));
	if (key == "NOTES") {	
	/*note values need to be analyzed here, because of different kind of 
	representation. Values are:
	<NotesType>:
	<Description>:
	<DifficultyClass>:
	<DifficultyMeter>:
	<RadarValues>:
	<NoteData>*/
	return false; // after taking note values, jumps to "NoteData"	
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

bool SongParser::smParseNote(std::string& line){
	if(line.empty()) return true;
	if(line[0] == '#') throw std::runtime_error("Key found in the middle of notes");
	if(line[0] == ';') return false; //end mark	

	//reading the notes into m_song.dance_notes
	//int lcount; 	//line counter for bpm
	int tm = 0; 	//time counter
	if(line[0] == ',') {
		/*TODO  Counting the timestamp of each note
			tm += 			
			count new bpm
			lcount =0;*/
		return true;
	}
	std::istringstream iss(line);
	std::map<int, Note> d_beat;
	for(int i =1; i <= 4; i++){
		if(iss.get() == '0') { 
			Note note;			
			//@TODO Note should be NULL or something...
			d_beat[i] = note;
			continue; 
		}
		else { //if(iss.get() == '1')
			Note note;		
			note.begin = tm;
			note.end = tm;
			d_beat[i] = note;
		}
		//@TODO hold etc. notes
	m_song.d_notes.push_back(d_beat); //d_notes must be in song.hh
	}
	return true;
}
