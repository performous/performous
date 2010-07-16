#pragma once

#include "song.hh"
#include "unicode.hh"
#include <sstream>
#include <boost/filesystem.hpp>

namespace SongParserUtil {
	/// Parse an int from string and assign it to a variable
	void assign(int& var, std::string const& str);
	/// Parse a double from string and assign it to a variable
	void assign(double& var, std::string str);
	/// Parse a boolean from string and assign it to a variable
	void assign(bool& var, std::string const& str);
	/// Erase last character if it matches
	void eraseLast(std::string& s, char ch = ' ');
}

/// parses songfiles
class SongParser {
  public:
	/// constructor
	SongParser(Song& s);
  private:
	void finalize();

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
	bool txtParseNote(std::string line, VocalTrack &vocal);
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
	Song::Stops m_stops;
	/// Convert a stop into <time, duration> (as stored in the song)
	std::pair<double, double> stopConvert(std::pair<double, double> s) {
		s.first = tsTime(s.first);
		return s;
	}
};
