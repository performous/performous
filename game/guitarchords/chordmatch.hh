#pragma once

#include "chord.hh"

#include <array>
#include <vector>

struct ChordMatch {
	Chord chord;
	std::array<std::vector<float>, 6> magnitudes;
	std::array<std::vector<float>, 6> inverseMagnitudes;

	ChordMatch(Chord const& chord)
	: chord(chord) {
	}
};

