#include "profilecreator.hh"
#include "chordmatcher.hh"
#include "normalizefilter.hh"

ProfileCreator& ProfileCreator::add(const Chord& chord, const std::vector<float>& sample) {
	m_samples[chord].push_back(sample);

	return *this;
}

Profile ProfileCreator::create(std::string const& name) const {
	auto matches = createMatches();

	analyze(matches);

	return Profile(name, {}, {});
}

void ProfileCreator::analyze(std::map<Chord, std::vector<std::vector<ChordMatch>>>& matches) const {
}

std::map<Chord, std::vector<std::vector<ChordMatch>>> ProfileCreator::createMatches() const {
	auto fft = FFT(48000, 8192*2);
	auto matches = std::map<Chord, std::vector<std::vector<ChordMatch>>>();

	for(auto const chord : m_samples) {
		for(auto const sample : chord.second) {
			auto buffer = sample;
			auto filter = Normalize(buffer);

			for(auto& v : buffer)
				v = filter(v);

			matches[chord.first].emplace_back();

			for(auto i = 0; i < 4; ++i) {
				auto const fftItems = fft.analyze(buffer);
				auto const match = ChordMatcher().calculate(fftItems, chord.first);

				matches[chord.first].back().emplace_back(match);
			}
		}
	}

	return matches;
}
