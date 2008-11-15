#pragma once
#ifndef PERFORMOUS_NOTES_HH
#define PERFORMOUS_NOTES_HH

#include <string>
#include <vector>

class MusicalScale {
	double m_baseFreq;
	static const int m_baseId = 33;
  public:
	MusicalScale(double baseFreq = 440.0): m_baseFreq(baseFreq) {}
	std::string getNoteStr(double freq) const;
	unsigned int getNoteNum(int id) const;
	bool isSharp(int id) const;
	double getNoteFreq(int id) const;
	int getNoteId(double freq) const;
	double getNote(double freq) const;
	double getNoteOffset(double freq) const;
};

struct Note {
	double begin, end;
	mutable double power;
	enum Type { FREESTYLE = 'F', NORMAL = ':', GOLDEN = '*', SLEEP = '-'} type;
	int note;
	std::string syllable;
	double diff(double n) const;
	double maxScore() const;
	double score(double freq, double b, double e) const;
  private:
	double scoreMultiplier(double error) const;
};

typedef std::vector<Note> Notes;

#endif
