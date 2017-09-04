#pragma once

/**
 * @file fft.hpp FFT and related facilities.
 */

#include "sample.hpp"
#include <complex>
#include <cstddef>
#include <vector>

extern const double m_pi;

const double m_tau = (2.0 * m_pi);

namespace da {

	// With g++ optimization -fcx-limited-range should be used for 5x performance boost.
	
	// Based on the description of Volodymyr Myrnyy in
	// http://www.dspdesignline.com/showArticle.jhtml?printableArticle=true&articleId=199903272
	template<unsigned P, typename T> struct DanielsonLanczos {
		static void apply(std::complex<T>* data) {
			constexpr std::size_t N = 1 << P;
			constexpr std::size_t M = N / 2;
			// Compute even and odd halves
			DanielsonLanczos<P - 1, T>().apply(data);
			DanielsonLanczos<P - 1, T>().apply(data + M);
			// Combine the results
			std::complex<T> w(1.0);
			for (std::size_t i = 0; i < M; ++i) {
				const std::complex<T> temp = data[i + M] * w;
				data[M + i] = data[i] - temp;
				data[i] += temp;
				w *= std::polar<T>(1.0, -m_tau / N);
			}
		}
	};

	template<typename T> struct DanielsonLanczos<0, T> { static void apply(std::complex<T>*) {} };

	/** Perform FFT on data. **/
	template<unsigned P, typename T> void fft(std::complex<T>* data) {
		// Perform bit-reversal sorting of sample data.
		constexpr std::size_t N = 1 << P;
		std::size_t j = 0;
		for (std::size_t i = 0; i < N; ++i) {
			if (i < j) std::swap(data[i], data[j]);
			std::size_t m = N / 2;
			while (m > 1 && m <= j) { j -= m; m >>= 1; }
			j += m;
		}
		// Do the actual calculation
		DanielsonLanczos<P, T>::apply(data);
	}

	/** Perform FFT on data from floating point iterator, windowing the input. **/
	template<unsigned P, typename InIt, typename Window> std::vector<std::complex<float> > fft(InIt begin, Window window) {
		std::vector<std::complex<float> > data(1 << P);
		// Perform bit-reversal sorting of sample data.
		constexpr std::size_t N = 1 << P;
		std::size_t j = 0;
		for (std::size_t i = 0; i < N; ++i) {
			data[j] = *begin++ * window[i];
			std::size_t m = N / 2;
			while (m > 1 && m <= j) { j -= m; m >>= 1; }
			j += m;
		}
		// Do the actual calculation
		DanielsonLanczos<P, float>::apply(&data[0]);
		return data;
	}

	template<unsigned P, typename T> void ifft(std::complex<T>* data) {
		constexpr std::size_t N = 1 << P;
		for (std::size_t i = 0; i < N; ++i) data[i] = std::conj(data[i]);  // Invert phase so that we can use FFT to do IFFT
		fft<P>(data);
		constexpr std::complex<T> scale(1.0/N, 0.0);
		for (std::size_t i = 0; i < N; ++i) data[i] = scale * std::conj(data[i]);  // Invert back, and apply IFFT scaling
	}
	
}
