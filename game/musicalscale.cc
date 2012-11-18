#include "musicalscale.hh"

#include <cmath>
#include <sstream>
#include <stdexcept>

// NOTE: This is the C major scale.
// The format needs to be preserved for isSharp, and any scale changes must be done in getNoteNum manually.
const char* const noteNames[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};

std::string MusicalScale::getNoteStr(double freq) const try {
	int id = getNoteId(freq);
	std::ostringstream oss;
	oss << noteNames[id % 12] << " " << int(freq + 0.5) << " Hz";
	return oss.str();
} catch (std::logic_error&) {
	return std::string();
}

unsigned int MusicalScale::getNoteNum(int id) const {
	// C major scale
	int n = id % 12;
	return (n + (n > 4)) / 2;
}

bool MusicalScale::isSharp(int id) const {
	if (id < 0) throw std::logic_error("MusicalScale::isSharp: Invalid note ID");
	return noteNames[id % 12][1] == '#';
}

double MusicalScale::getNoteFreq(int id) const {
	if (id == -1) return 0.0;
	return m_baseFreq * std::pow(2.0, (id - m_baseId) / 12.0);
}

int MusicalScale::getNoteId(double freq) const {
	double note = getNote(freq);
	return int(note + 0.5);  // Mathematical rounding
}

double MusicalScale::getNote(double freq) const {
	double note = m_baseId + 12.0 * std::log(freq / m_baseFreq) / std::log(2.0);
	if (note >= 0.0 && note <= 127.0) return note;
	throw std::logic_error("MusicalScale::getNote: Invalid freq");
}

double MusicalScale::getNoteOffset(double freq) const {
	double note = getNote(freq);
	return note - int(note + 0.5);
}

