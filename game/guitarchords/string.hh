#pragma once

#include "notes.hh"

#include <vector>

using Fret = unsigned char;

static constexpr Fret FretNone = 255;

class String {
public:
	String(float frequency);

	float getFrequency(Fret fret, bool inverse = false) const;
	bool hasInverseFrequency(Fret fret) const;

private:
	float m_frequency;
};

class StringPlay {
public:
	StringPlay(String const& string, Fret const& fret);

	float getFrequency(bool inverse = false) const;
	bool hasInverseFrequency() const;

	bool operator<(StringPlay const&) const;

private:
	String m_string;
	Fret m_fret;
};

std::vector<String> const strings{E0, a0, d0, g0, b0, e0};
