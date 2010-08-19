#pragma once

/**
 * @file fft.hpp FFT and related facilities.
 */

#include <complex>
#include <cstddef>
#include <vector>

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

namespace da {

	namespace math {

		/** Calculate the square of val. **/
		static inline double sqr(double val) { return val * val; }

		template <unsigned M, unsigned N, unsigned B, unsigned A> struct SinCosSeries {
			static double value() {
				return 1 - sqr(A * M_PI / B) / M / (M+1) * SinCosSeries<M + 2, N, B, A>::value();
			}
		};
		template <unsigned N, unsigned B, unsigned A> struct SinCosSeries<N, N, B, A> {
			static double value() { return 1.0; }
		};

		template <unsigned A, unsigned B> struct Sin {
			static double value() {	return (A * M_PI / B) * SinCosSeries<2, 34, B, A>::value(); }
		};

		template <unsigned A, unsigned B> struct Cos {
			static double value() { return SinCosSeries<1, 33, B, A>::value(); }
		};

		/** Calculate sin(2 pi A / B). **/
		template <unsigned A, unsigned B> double sin() { return Sin<A, B>::value(); }

		/** Calculate cos(2 pi A / B). **/
		template <unsigned A, unsigned B> double cos() { return Cos<A, B>::value(); }
	}

	namespace fourier {
		// Based on the description of Volodymyr Myrnyy in
		// http://www.dspdesignline.com/showArticle.jhtml?printableArticle=true&articleId=199903272
		template<unsigned P, typename T> struct DanielsonLanczos {
			static void apply(std::complex<T>* data) {
				const std::size_t N = 1 << P;
				const std::size_t M = N / 2;
				// Compute even and odd halves
				DanielsonLanczos<P - 1, T>().apply(data);
				DanielsonLanczos<P - 1, T>().apply(data + M);
				// Combine the results
				using math::sqr;
				using math::sin;
				const std::complex<T> wp(-2.0 * sqr(sin<1, N>()), -sin<2, N>());
				std::complex<T> w(1.0);
				for (std::size_t i = 0; i < M; ++i) {
					std::complex<T> temp = data[i + M] * w;
					data[M + i] = data[i] - temp;
					data[i] += temp;
					w += w * wp;
				}
			}
		};

		template<typename T> struct DanielsonLanczos<0, T> { static void apply(std::complex<T>*) {} };
	}

	/** Perform FFT on data. **/
	template<unsigned P, typename T> void fft(std::complex<T>* data) {
		// Perform bit-reversal sorting of sample data.
		const std::size_t N = 1 << P;
		std::size_t j = 0;
		for (std::size_t i = 0; i < N; ++i) {
			if (i < j) std::swap(data[i], data[j]);
			std::size_t m = N / 2;
			while (m > 1 && m <= j) { j -= m; m >>= 1; }
			j += m;
		}
		// Do the actual calculation
		fourier::DanielsonLanczos<P, T>::apply(data);
	}

	/** Perform FFT on data from floating point iterator, windowing the input. **/
	template<unsigned P, typename InIt, typename Window> std::vector<std::complex<float> > fft(InIt begin, Window window) {
		std::vector<std::complex<float> > data(1 << P);
		// Perform bit-reversal sorting of sample data.
		const std::size_t N = 1 << P;
		std::size_t j = 0;
		for (std::size_t i = 0; i < N; ++i) {
			data[j] = *begin++ * window[i];
			std::size_t m = N / 2;
			while (m > 1 && m <= j) { j -= m; m >>= 1; }
			j += m;
		}
		// Do the actual calculation
		fourier::DanielsonLanczos<P, float>::apply(&data[0]);
		return data;
	}

}
