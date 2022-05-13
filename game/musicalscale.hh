#pragma once

#include <cstdint>
#include <cmath>
#include <string>

/// Conversions for the C major musical scale
class MusicalScale {
  private:
	double m_baseFreq;
	static const int m_baseId = 69;  ///< MIDI note that corresponds to baseFreq
	double m_freq;
	double m_note;
  public:
	MusicalScale(double baseFreq = 440.0): m_baseFreq(baseFreq) { clear(); }  ///< Construct a C major scale (no others are currently implemented)
	MusicalScale& clear();  ///< Clear current note/freq values
	MusicalScale& setFreq(double freq);  ///< Set note by frequency
	MusicalScale& setNote(double note);  ///< Set note by note value
	double getFreq() const { return m_freq; }  ///< Get the note frequency
	double getNote() const { return m_note; }  ///< Get the precise (non-rounded) note id for a the frequency
	float getNoteId() const;   ///< Get the nearest note for the frequency
	double getNoteOffset() const { return m_note - getNoteId(); }  ///< Get the note offset (-0.5 to 0.5)
	std::string getStr() const;  ///< Get a human-readable string representation for the note
	unsigned getNum() const { return static_cast<unsigned>(std::fmod(getNoteId(), 12.0f)); }  ///< Get note number within an octave (0 to 11, begins at key, e.g. C)
	int getOctave() const { return static_cast<int>(std::floor(getNoteId() / 12.0f - 1.0f)); }   ///< Get the octave number (-1 to 9, A4 ~ baseFreq)
	unsigned getNoteLine() const;  ///< Get a note line number in traditional notation (0 ~ key base e.g. C4, odd values are between lines, negative for lower octaves)
	bool isSharp() const;  ///< Check if the note is sharp (#)
	bool isValid() const { return m_note >= 0.0 && m_note <= 127.0; }  ///< Test if a valid note value is stored
};
