#pragma once

#include <memory>
#include <vector>

//#include "aubio/aubio.h"

struct FFTItem {
	float frequency;
	float power;
};

class FFT {
  public:
	FFT(std::size_t sampleRate, std::size_t windowSize);

	std::vector<FFTItem> analyse(std::vector<float> const&) const;

  private:
//    std::unique_ptr<aubio_fft_t, void(*)(aubio_fft_t*)> m_fft;
//    std::unique_ptr<fvec_t, void(*)(fvec_t*)> m_input;
//    std::unique_ptr<cvec_t, void(*)(cvec_t*)> m_output;
	std::size_t m_sampleRate;
	std::size_t m_windowSize;
	std::vector<float> m_window;
};
