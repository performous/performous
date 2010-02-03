#pragma once

#include "song.hh"
#include "unicode.hh"
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

/// parses songfiles
class SongParser {
  public:
	/// constructor
	SongParser(Song& s):
	  m_song(s),
	  m_linenum(),
	  m_relative(),
	  m_gap(),
	  m_bpm(),
	  m_prevtime(),
	  m_prevts(),
	  m_relativeShift(),
	  m_maxScore(),
	  m_tsPerBeat(),
	  m_tsEnd()
	{
		enum { NONE, TXT, INI, SM } type = NONE;
		// Read the file, determine the type and do some initial validation checks
		{
			std::ifstream f((s.path + s.filename).c_str(), std::ios::binary);
			if (!f.is_open()) throw SongParserException("Could not open song file", 0);
			f.seekg(0, std::ios::end);
			size_t size = f.tellg();
			if (size < 10 || size > 100000) throw SongParserException("Does not look like a song file (wrong size)", 1, true);
			f.seekg(0);
			std::vector<char> data(size);
			if (!f.read(&data[0], size)) throw SongParserException("Unexpected I/O error", 0);
			if (smCheck(data)) type = SM;
			else if (txtCheck(data)) type = TXT;
			else if (iniCheck(data)) type = INI;
			else throw SongParserException("Does not look like a song file (wrong header)", 1, true);
			m_ss.write(&data[0], size);
		}
		convertToUTF8(m_ss, s.path + s.filename);
		// Header already parsed?
		if (s.loadStatus == Song::HEADER) {
			try {
				if (type == TXT) txtParse();
				else if (type == INI) iniParse();
				else if (type == SM) smParse();
			} catch (std::runtime_error& e) {
				throw SongParserException(e.what(), m_linenum);
			}
			finalize(); // Do some adjusting to the notes
			s.loadStatus = Song::FULL;
			return;
		}
		// Parse only header to speed up loading and conserve memory
		try {
			if (type == TXT) txtParseHeader();
			else if (type == INI) iniParseHeader();
			else if (type == SM) { smParseHeader(); s.dropNotes(); } // Hack: drop notes here
		} catch (std::runtime_error& e) {
			throw SongParserException(e.what(), m_linenum);
		}

		// Remove bogus entries
		if (!boost::filesystem::exists(m_song.path + m_song.cover)) m_song.cover = "";
		if (!boost::filesystem::exists(m_song.path + m_song.background)) m_song.background = "";
		if (!boost::filesystem::exists(m_song.path + m_song.video)) m_song.video = "";

		// In case no images/videos were specified, try to guess them
		if (m_song.cover.empty() || m_song.background.empty() || m_song.video.empty()) {
			boost::regex coverfile("((cover|album|label|\\[co\\])\\.(png|jpeg|jpg|svg))$", boost::regex_constants::icase);
			boost::regex backgroundfile("((background|bg||\\[bg\\])\\.(png|jpeg|jpg|svg))$", boost::regex_constants::icase);
			boost::regex videofile("(.*\\.(avi|mpg|mpeg|flv|mov|mp4))$", boost::regex_constants::icase);
			boost::cmatch match;

			for (boost::filesystem::directory_iterator dirIt(s.path), dirEnd; dirIt != dirEnd; ++dirIt) {
				boost::filesystem::path p = dirIt->path();
				std::string name = p.leaf(); // File basename
				if (m_song.cover.empty() && regex_match(name.c_str(), match, coverfile)) {
					m_song.cover = name;
				} else if (m_song.background.empty() && regex_match(name.c_str(), match, backgroundfile)) {
					m_song.background = name;
				} else if (m_song.video.empty() && regex_match(name.c_str(), match, videofile)) {
					m_song.video = name;
				}
			}
		}
		s.loadStatus = Song::HEADER;
	}
  private:
	void finalize() {
		// Adjust negative notes
		if (m_song.noteMin <= 0) {
			unsigned int shift = (1 - m_song.noteMin / 12) * 12;
			m_song.noteMin += shift;
			m_song.noteMax += shift;
			for (Notes::iterator it = m_song.notes.begin(); it != m_song.notes.end(); ++it) {
				it->note += shift;
				it->notePrev += shift;
			}
		}
		// Set begin/end times
		if (!m_song.notes.empty()) m_song.beginTime = m_song.notes.front().begin, m_song.endTime = m_song.notes.back().end;
		m_song.m_scoreFactor = 1.0 / m_maxScore;
		if (m_tsPerBeat) {
			// Add song beat markers
			for (unsigned ts = 0; ts < m_tsEnd; ts += m_tsPerBeat) m_song.beats.push_back(tsTime(ts));
		}
	}

	Song& m_song;
	std::stringstream m_ss;
	unsigned int m_linenum;
	bool getline(std::string& line) { ++m_linenum; return std::getline(m_ss, line);}
	bool m_relative;
	double m_gap;
	double m_bpm;
	
	bool txtCheck(std::vector<char> const& data);
	void txtParseHeader();
	void txtParse();
	bool txtParseField(std::string const& line);
	bool txtParseNote(std::string line);
	bool iniCheck(std::vector<char> const& data);
	void iniParseHeader();
	void iniParse();
	bool smCheck(std::vector<char> const& data);
	void smParseHeader();
	void smParse();
	bool smParseField(std::string line);
	Notes smParseNotes(std::string line);
	double m_prevtime;
	unsigned int m_prevts;
	unsigned int m_relativeShift;
	double m_maxScore;
	struct BPM {
		BPM(double _begin, double _ts, double bpm): begin(_begin), step(0.25 * 60.0 / bpm), ts(_ts) {}
		double begin; // Time in seconds
		double step; // Seconds per quarter note
		double ts;
	};
	typedef std::vector<BPM> bpms_t;
	bpms_t m_bpms;
	unsigned m_tsPerBeat;  ///< The ts increment per beat
	unsigned m_tsEnd;  ///< The ending ts of the song
	void addBPM(double ts, double bpm) {
		if (!(bpm >= 1.0 && bpm < 1e12)) throw std::runtime_error("Invalid BPM value");
		if (!m_bpms.empty() && m_bpms.back().ts >= ts) {
			if (m_bpms.back().ts < ts) throw std::runtime_error("Invalid BPM timestamp");
			m_bpms.pop_back(); // Some ITG songs contain repeated BPM definitions...
		}
		m_bpms.push_back(BPM(tsTime(ts), ts, bpm));
	}
	/// Convert a timestamp (beats) into time (seconds)
	double tsTime(double ts) const {
		if (m_bpms.empty()) {
			if (ts != 0) throw std::runtime_error("BPM data missing");
			return m_gap;
		}
		for (std::vector<BPM>::const_reverse_iterator it = m_bpms.rbegin(); it != m_bpms.rend(); ++it) {
			if (it->ts <= ts) return it->begin + (ts - it->ts) * it->step;
		}
		throw std::logic_error("INTERNAL ERROR: BPM data invalid");
	}
	/// Stops stored in <ts, duration> format
	std::vector<std::pair<double, double> > m_stops;
	/// Convert a stop into <time, duration> (as stored in the song)
	std::pair<double, double> stopConvert(std::pair<double, double> s) {
		s.first = tsTime(s.first);
		return s;
	}
};

