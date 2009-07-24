#pragma once
#ifndef PERFORMOUS_FFTGRAPH_HH
#define PERFORMOUS_FFTGRAPH_HH

#include <fft.hpp>
#include <fstream>
#include <deque>
#include <cmath>
#include "ffmpeg.hh"

#define FFT_P 13
#define FFT_N (1 << FFT_P)

struct FFTGraph {
	std::deque<std::vector<std::complex<float> > > m_data;
	FFTGraph(CFfmpeg& ff) {
		std::vector<float> audio;
		while (!ff.audioQueue.eof()) {
			std::vector<int16_t> a;
			ff.audioQueue.tryPop(a);
			for (size_t i = 0; i < a.size(); i += 2) audio.push_back(da::conv_from_s16(a[i]) + da::conv_from_s16(a[i+1]));
			std::transform(a.begin(), a.end(), std::back_inserter(audio), da::conv_from_s16);
			usleep(100000);
		};
		ff.seek(0.0);
		std::vector<float> m_window(FFT_N);
	  	// Hamming window
		for (size_t i=0; i < FFT_N; i++) {
			m_window[i] = 0.53836 - 0.46164 * std::cos(2.0 * M_PI * i / (FFT_N - 1));
		}
		std::ofstream f("test123.bin", std::ios::binary);
		for (size_t i = 0; i + FFT_N < audio.size(); i += 2000) {
			//m_data.push_back(da::fft<FFT_P>(audio.begin() + i, m_window));
			std::vector<std::complex<float> > fft = da::fft<FFT_P>(audio.begin() + i, m_window);
			for (size_t j = 15; j < 130; ++j) {
				unsigned char ch = 127.0 + std::min(127.0, std::max(-127.0, 30.0 * (std::log(std::abs(fft[j])) - 3.0)));
				f.put(ch);
				f.put(ch);
				f.put(ch);
			}
		}
	}
};

#endif

