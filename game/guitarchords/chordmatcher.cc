#include "chordmatcher.hh"


ChordMatch ChordMatcher::calculate(std::vector<FFTItem> const& fftItems, Chord const& chord) {
	auto const binWidth = static_cast<float>(48000) / static_cast<float>(8192*2);
	auto match = ChordMatch(chord);

	for(auto i = 0; i < 6; ++i) {
		auto const fret = chord.getFrets()[i];

		if(fret == FretNone)
			continue;

		auto const frequency = strings[i].getFrequency(fret);
		auto const frequencyMagnitude = fftItems[static_cast<std::size_t>(frequency / binWidth)].power;

		match.magnitudes[i].emplace_back(frequencyMagnitude);

		for(auto const multiplier : m_overNoteMultiplier) {
			auto const overnoteFrequency = frequency * multiplier;
			auto const overnoteMagnitude = fftItems[static_cast<std::size_t>(overnoteFrequency / binWidth)].power;

			match.magnitudes[i].emplace_back(overnoteMagnitude);
		}

		if(strings[i].hasInverseFrequency(fret)) {
			auto const inverseFrequency = strings[i].getFrequency(fret, true);
			auto const inverseFrequencyBin = static_cast<std::size_t>(inverseFrequency / binWidth);
			auto const inverseFrequencyMagnitude = fftItems[inverseFrequencyBin].power;

			match.inverseMagnitudes[i].emplace_back(inverseFrequencyMagnitude);

			for(auto const multiplier : m_overNoteMultiplier) {
				auto const overnoteFrequency = inverseFrequency * multiplier;
				auto const overnoteMagnitude = fftItems[static_cast<std::size_t>(overnoteFrequency / binWidth)].power;

				match.inverseMagnitudes[i].emplace_back(overnoteMagnitude);
			}
		}
		else {
			//std::cout << "No inverse frequency " << std::endl;
			match.inverseMagnitudes[i].emplace_back(0.f);
			for(auto const multiplier : m_overNoteMultiplier)
				match.inverseMagnitudes[i].emplace_back(0.f);
		}
	}

	return match;
}

