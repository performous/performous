#pragma once

#include <string>

/// Conversions for the C major musical scale
class MusicalScale {
  private:
	  float m_baseFreq;
	static const int m_baseId = 69;  ///< MIDI note that corresponds to baseFreq
	float m_freq;
	float m_note;
  public:
	MusicalScale(float baseFreq = 440.0f): m_baseFreq(baseFreq) { clear(); }  ///< Construct a C major scale (no others are currently implemented)
	MusicalScale& clear();  ///< Clear current note/freq values
	MusicalScale& setFreq(float freq);  ///< Set note by frequency
	MusicalScale& setNote(float note);  ///< Set note by note value
	float getFreq() const { return m_freq; }  ///< Get the note frequency
	float getNote() const { return m_note; }  ///< Get the precise (non-rounded) note id for a the frequency
	unsigned getNoteId() const;   ///< Get the nearest note for the frequency
	float getNoteOffset() const { return m_note - getNoteId(); }  ///< Get the note offset (-0.5 to 0.5)
	std::string getStr() const;  ///< Get a human-readable string representation for the note
	unsigned getNum() const { return getNoteId() % 12; }  ///< Get note number within an octave (0 to 11, begins at key, e.g. C)
	int getOctave() const { return int(getNoteId() / 12) - 1; }   ///< Get the octave number (-1 to 9, A4 ~ baseFreq)
	int getNoteLine() const;  ///< Get a note line number in traditional notation (0 ~ key base e.g. C4, odd values are between lines, negative for lower octaves)
	bool isSharp() const;  ///< Check if the note is sharp (#)
	bool isValid() const { return m_note >= 0.0f && m_note <= 127.0f; }  ///< Test if a valid note value is stored
};

