#pragma once

#include "chord.hh"
#include "fft.hh"

#include <memory>
#include <vector>
#include "biquadfilter.hh"
#include "profile.hh"

struct ISignal {
	virtual ~ISignal() = default;

	virtual void readTo(std::vector<float>& buffer) = 0;
};

class ChordDetector {
public:
	ChordDetector(Profile const&);

	bool detect(Chord const&, ISignal&) const;

private:
	Profile const m_profile;
	FFT m_fft;
	mutable std::vector<float> m_buffer;
};
