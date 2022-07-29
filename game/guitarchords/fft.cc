#include "fft.hh"

#include "util.hh"

#include <cstring>
#include <stdexcept>
#include <cmath>
#include <libda/fft.hpp>

FFT::FFT(std::size_t sampleRate, std::size_t windowSize)
:
//m_fft(new_aubio_fft(windowSize), del_aubio_fft),
//m_input(new_fvec(windowSize), del_fvec),
//m_output(new_cvec(windowSize), del_cvec),
m_sampleRate(sampleRate),
m_windowSize(windowSize),
m_window(windowSize)
{
	createHammingWindow(m_window.begin(), m_window.end());
}

namespace {
	std::vector<std::complex<float>> doFFT(const std::vector<float>& input, const std::vector<float>& window) {
		if(window.size() == 16384)
			return da::fft<14>(input.data(), window);
		if(window.size() == 8192)
			return da::fft<13>(input.data(), window);
		if(window.size() == 4096)
			return da::fft<12>(input.data(), window);
		if(window.size() == 2048)
			return da::fft<11>(input.data(), window);
		if(window.size() == 1024)
			return da::fft<10>(input.data(), window);
		if(window.size() == 512)
			return da::fft<9>(input.data(), window);

		throw std::logic_error("doFFT: window of size " + std::to_string(window.size()) + " is not supported!");
	}
}

std::vector<FFTItem> FFT::analyze(const std::vector<float>& input) const {
	if(input.size() != m_windowSize)
		throw std::logic_error("FFT: input must match window size!");
/*
	std::memcpy(m_input->data, input.data(), m_windowSize * sizeof(float));

	auto filter = new_aubio_filter_c_weighting(48000);

	aubio_filter_do(filter, m_input.get());
	del_aubio_filter(filter);

	auto output = new_fvec(m_windowSize);
	aubio_fft_do_complex(m_fft.get(), m_input.get(), output);

	auto result = std::vector<FFTItem>(output->length / 2);

	for(auto i = decltype(output->length)(0); i < output->length / 2; ++i) {
		auto const real = output->data[i * 2];
		auto const imag = output->data[i * 2 + 1];

		result[i].power = std::sqrt(real * real + imag * imag);
		result[i].frequency = float(i * m_sampleRate) / float(m_windowSize);
	}

	del_fvec(output);
*/

	auto const output = doFFT(input, m_window);
	auto result = std::vector<FFTItem>(output.size() / 2);

	for(auto i = decltype(output.size())(0); i < output.size() / 2; ++i) {
		auto const real = output[i].real();
		auto const imag = output[i].imag();

		result[i].power = std::sqrt(real * real + imag * imag);
		result[i].frequency = float(i * m_sampleRate) / float(m_windowSize);
	}

	return result;
}

