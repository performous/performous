#include "chorddetector.hh"

#include "fft.hh"
#include "chords.hh"
#include "chordmatch.hh"

#include <cmath>
#include <map>
#include <vector>

#include <iostream>
#include "normalizefilter.hh"
#include "chordmatcher.hh"

namespace {

	float rate_simple(ChordMatch const& match) {
		auto score = 0.f;

		for(auto i = 0U; i < 6U; ++i) {
			if(match.chord.getFrets()[i] == FretNone)
				continue;

			for(auto const magnitude : match.magnitudes[i])
				score += magnitude;

			for(auto const magnitude : match.inverseMagnitudes[i])
				score += magnitude;
		}

		return score;
	}

	float rate_db_normalized(ChordMatch const& match) {
		auto score = 0.f;

		for(auto i = 0U; i < 6U; ++i) {
			if(match.chord.getFrets()[i] == FretNone)
				continue;

			score += match.magnitudes[i][0];

			if(match.inverseMagnitudes[i][0] > 0.f) {
				auto const fretCount = match.chord.getFrets()[i] - 1;
				auto const normalizedMagnitude = std::min(match.inverseMagnitudes[i][0] , match.magnitudes[i][0]);
				auto const magnitude = normalizedMagnitude * pow((19 - fretCount), 1.25) / float(fretCount);

				std::cout << "for fret " << unsigned(match.chord.getFrets()[i]) << " from " << match.inverseMagnitudes[i][0] << " to " << magnitude << std::endl;
				score += magnitude;
			}

			for(auto j = 1; j < match.magnitudes[i].size(); ++j) {
				score += match.magnitudes[i][j];
			}
		}

		return score;
	}

	std::vector<ChordMatch> sort(std::vector<ChordMatch> const& matches) {
		auto result = std::vector<ChordMatch>();
		auto sorted = std::map<float, ChordMatch>();

		for(auto const& match : matches)
			sorted.emplace(-rate_db_normalized(match), match);

		for(auto const& [rate, match] : sorted)
			result.emplace_back(match);

		return result;
	}
}

ChordDetector::ChordDetector(Profile const& profile)
: m_profile(profile), m_fft(48000, 8192*2), m_buffer(8192*2) {
}

bool ChordDetector::detect(const Chord& chord, ISignal& signal) const {
	signal.readTo(m_buffer);

	//auto filter = BiQuad();
	auto filter2 = Normalize(m_buffer);

	for(auto& v : m_buffer)
		//v = filter(filter2(v));
		v = filter2(v);

	auto const result = m_fft.analyze(m_buffer);
	auto matches = std::vector<ChordMatch>();

	for(auto const& chordToCheck : chords) {
		matches.emplace_back(ChordMatcher().calculate(result, chordToCheck));
		m_profile.adjust(matches.back());
	}

	auto const sorted = sort(matches);

	return chord == sorted[0].chord;
}

