#include "tone.hh"

#include "log.hh"
#include "util.hh"

#include <cmath>

Tone::Tone():
  db(-getInf()), stabledb(-getInf()) {
	for (auto& h: harmonics) h = -getInf();
}

void Tone::print() const {
	if (age < Tone::MINAGE)
		return;
	std::string ret{fmt::format("Tone freq={:.1f} Hz, age={}, loudnesss={:.1f} dB. Harmonics: ", freq, age, db)};
	for (std::size_t i = 0; i < 8; ++i) {
		fmt::format_to(std::back_inserter(ret), fmt::runtime("{:.1f} Hz{}"), harmonics[i], i < 7 ? ", " : ".");
	}
	SpdLogger::info(LogSystem::ENGINE, ret);
}

bool Tone::operator==(double f) const {
	return std::abs(freq / f - 1.0) < 0.05;
}


