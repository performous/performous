#include "musicalscale.hh"

#include "util.hh"
#include <cmath>
#include <sstream>
#include <stdexcept>

MusicalScale& MusicalScale::clear() { m_freq = m_note = getNaN(); return *this; }

MusicalScale& MusicalScale::setFreq(float freq) {
	m_freq = freq;
	m_note = m_baseId + 12.0f * std::log(freq / m_baseFreq) / std::log(2.0f);
	if (!isValid()) m_note = getNaN();
	return *this;
}

MusicalScale& MusicalScale::setNote(float note) {
	m_note = note;
	m_freq = m_baseFreq * std::pow(2.0f, (m_note - m_baseId) / 12.0f);
	return *this;
}

unsigned MusicalScale::getNoteId() const {
	if (!isValid()) throw std::logic_error("MusicalScale::getNoteId must only be called on valid notes.");
	return round(m_note);
}

// NOTE: Only C major scale is currently implemented.

static const char* const noteNames[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
static const int noteLines[12] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };

std::string MusicalScale::getStr() const {
	if (!isValid()) return std::string();
	std::ostringstream oss;
	oss << noteNames[getNum()] << " " << round(m_freq) << " Hz";
	return oss.str();
}

int MusicalScale::getNoteLine() const {
	return (getOctave() - 4) * 7 + noteLines[getNum()];
}

bool MusicalScale::isSharp() const {
	return noteNames[getNum()][1] == '#';  // Uses the second character of noteNames!
}

