#include "tone.hh"

#include "util.hh"
#include <cmath>
#include <iostream>
#include <iomanip>

Tone::Tone():
  db(-getInf()), stabledb(-getInf()) {
	for (auto& h: harmonics) h = -getInf();
}

void Tone::print() const {
	if (age < Tone::MINAGE)
		return;
	std::cout << std::fixed << std::setprecision(1) << freq << " Hz, age " << age << ", " << db << " dB:";
	for (std::size_t i = 0; i < 8; ++i)
		std::cout << " " << harmonics[i];
	std::cout << std::endl;
}

void Tone::print(std::ostream& os) const {
	os << std::fixed << std::setprecision(1) << freq << " Hz, age " << age << ", " << db << " dB:";
	for (std::size_t i = 0; i < 8; ++i)
		os << " " << harmonics[i];
}

bool Tone::operator==(double f) const {
	return std::abs(freq / f - 1.0) < 0.05;
}


