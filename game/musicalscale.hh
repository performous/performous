#pragma once

#include <string>

/// Conversions for the C major musical scale
class MusicalScale {
  private:
	double m_baseFreq;
	static const int m_baseId = 69;  ///< What is the baseFreq in MIDI?

  public:
	/// Construct a scale object
	MusicalScale(double baseFreq = 440.0): m_baseFreq(baseFreq) {}
	/// Get a human-readable string representation for the frequency
	std::string getNoteStr(double freq) const;
	/// Get a note line number in traditional notation (0 = C, 1 = D, ...)
	unsigned int getNoteNum(int id) const;
	/// Check if the note is sharp (#)
	bool isSharp(int id) const;
	/// Get the proper frequency of the note
	double getNoteFreq(int id) const;
	/// Get the nearest note for the frequency
	int getNoteId(double freq) const;
	/// Get the precise (non-rounded) note id for a the frequency
	double getNote(double freq) const;
	/// Get the offset (-0.5 to 0.5) from the nearest note
	double getNoteOffset(double freq) const;
};

