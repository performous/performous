#pragma once

#include "chord.hh"
#include "chordmatch.hh"
#include "fft.hh"

#include <array>
#include <vector>

class ChordMatcher {
public:
	ChordMatch calculate(std::vector<FFTItem> const& fftItems, Chord const& chord);

private:
	std::array<float, 7> m_overNoteMultiplier{2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
};
