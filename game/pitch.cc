#include "pitch.hh"

#include "util.hh"
#include "libda/fft.hpp"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdexcept>

// Limit the range to avoid noise and useless computation
static const double FFT_MINFREQ = 45.0;
static const double FFT_MAXFREQ = 5000.0;

Tone::Tone():
  freq(0.0),
  db(-getInf()),
  stabledb(-getInf()),
  age()
{
	for (auto& h: harmonics) h = -getInf();
}

void Tone::print() const {
	if (age < Tone::MINAGE) return;
	std::cout << std::fixed << std::setprecision(1) << freq << " Hz, age " << age << ", " << db << " dB:";
	for (std::size_t i = 0; i < 8; ++i) std::cout << " " << harmonics[i];
	std::cout << std::endl;
}

bool Tone::operator==(double f) const {
	return std::abs(freq / f - 1.0) < 0.05;
}

Analyzer::Analyzer(double rate, std::string id, std::size_t step):
  m_step(step),
  m_resampleFactor(1.0),
  m_resamplePos(),
  m_rate(rate),
  m_id(id),
  m_window(FFT_N),
  m_fftLastPhase(FFT_N / 2),
  m_peak(0.0),
  m_oldfreq(0.0)
{
	if (m_step > FFT_N) throw std::logic_error("Analyzer step is larger that FFT_N (ideally it should be less than a fourth of FFT_N).");
	for (size_t i=0; i < FFT_N; i++) {
		// Hamming window with unitary power
		m_window[i] = 0.613 * (0.53836 - 0.46164 * std::cos(TAU * i / (FFT_N - 1)));
		// FFT normalization (preserving signal power)
		m_window[i] *= std::sqrt(2.0) / FFT_N;
	}
}

void Analyzer::output(float* begin, float* end, double rate) {
	constexpr unsigned a = 2;
	const unsigned size = m_passthrough.size();
	const unsigned out = (end - begin) / 2 /* stereo */;
	if (out == 0) return;
	const unsigned in = m_resampleFactor * (m_rate / rate) * out + 2 * a /* lanczos kernel */ + 5 /* safety margin for rounding errors */;
	float pcm[m_passthrough.capacity];
	m_passthrough.read(pcm, pcm + in + 4);
	for (unsigned i = 0; i < out; ++i) {
		double s = 0.0;
		unsigned k = m_resamplePos;
		double x = m_resamplePos - k;
		// Lanczos sampling of input at m_resamplePos
		for (unsigned j = 0; j <= 2 * a; ++j) s += pcm[k + j] * da::lanc<a>(x - j + a);
		s *= 5.0;
		begin[i * 2] += s;
		begin[i * 2 + 1] += s;
		m_resamplePos += m_resampleFactor;
	}
	unsigned num = m_resamplePos;
	m_resamplePos -= num;
	if (size > 3000) {
		// Reset
		m_passthrough.pop(m_passthrough.size() - 700);
		m_resampleFactor = 1.0;
	} else {
		m_passthrough.pop(num);
		m_resampleFactor = 0.99 * m_resampleFactor + 0.01 * (size > 700 ? 1.02 : 0.98);
	}
}


namespace {
	struct Peak {
		double freq;
		double db;
		bool harm[Tone::MAXHARM];
		Peak(double _freq = 0.0, double _db = -getInf()):
		  freq(_freq), db(_db)
		{
			for (auto& h: harm) h = false;
		}
		void clear() {
			freq = 0.0;
			db = -getInf();
		}
	};

	Peak& match(std::vector<Peak>& peaks, double freq, double freqPerBin) {
		std::size_t best = std::floor(freq / freqPerBin);
		if (best + 2 > peaks.size()) return peaks[0];
		double d0 = std::abs(peaks[best].freq - freq);
		double d1 = std::abs(peaks[best + 1].freq - freq);
		return d0 < d1 ? peaks[best] : peaks[best + 1];
	}
}

bool Analyzer::calcFFT() {
	float pcm[FFT_N];
	// Read FFT_N samples, move forward by m_step samples
	if (!m_buf.read(pcm, pcm + FFT_N)) return false;
	m_buf.pop(m_step);
	// Peak level calculation of the most recent m_step samples (the rest is overlap)
	for (float const* ptr = pcm + FFT_N - m_step; ptr != pcm + FFT_N; ++ptr) {
		float s = *ptr;
		float p = s * s;
		if (p > m_peak) m_peak = p; else m_peak *= 0.999;
	}
	// Calculate FFT
	m_fft = da::fft<FFT_P>(pcm, m_window);
	return true;
}

void Analyzer::calcTones() {
	// Precalculated constants
	const double freqPerBin = m_rate / FFT_N;
	const double stepRate = m_rate / m_step;  // Steps per second
	const double phaseStep = double(m_step) / FFT_N;
	const double minMagnitude = pow(10, -80.0 / 20.0); // -80 dB
	// Limit frequency range of processing
	const size_t kMin = std::max(size_t(1), size_t(FFT_MINFREQ / freqPerBin));
	const size_t kMax = std::min(FFT_N / 2, size_t(FFT_MAXFREQ / freqPerBin));
	std::map<double, double> freqs;  // <freq, magnitude>
	for (size_t k = 1; k <= kMax; ++k) {
		double magnitude = std::abs(m_fft[k]);
		double phase = std::arg(m_fft[k]) / TAU;
		double delta = phase - m_fftLastPhase[k];
		m_fftLastPhase[k] = phase;
		// Use phase difference over a step to calculate what the frequency must be
		double freq = stepRate * (std::round(k * phaseStep - delta) + delta);
		if (magnitude > minMagnitude && std::abs(freq / freqPerBin - k) <= 0.7) {
			freqs[freq] = magnitude;
		}
	}
	// Group multiple bins' identical frequencies together by total power
	double freqSum = 0.0, powerSum = 0.0;
	std::vector<Peak> peaks(kMax + 1);	// One extra to simplify loops
	for (auto const& kv: freqs) {
		double f = kv.first;
		double p = kv.second * kv.second;
		if (powerSum > 0.0 && abs(f - freqSum / powerSum) > 10.0 /* Hz */) {
			freqSum /= powerSum;
			size_t k = freqSum / freqPerBin;
			if (k >= 1 && k < kMax) {
				double db = 10.0 * log10(powerSum);
				if (db > peaks[k].db) {
					peaks[k].freq = freqSum;
					peaks[k].db = db;
				}
			}
			freqSum = powerSum = 0.0;
		}
		freqSum += p * f;
		powerSum += p;
	}

	// Find the tones (collections of harmonics) from the array of peaks
	tones_t tones;
	for (size_t k = kMax - 1; k >= kMin; --k) {
		if (peaks[k].db < -60.0) continue;
		// Find the best divider for getting the fundamental from peaks[k]
		std::size_t bestDiv = 1;
		int bestScore = 0;
		for (std::size_t div = 2; div <= Tone::MAXHARM && k / div > 1; ++div) {
			double freq = peaks[k].freq / div; // Fundamental
			int score = 0;
			for (std::size_t n = 1; n < div && n < 8; ++n) {
				Peak& p = match(peaks, n * freq, freqPerBin);
				if (std::abs(p.freq / n / freq - 1.0) > .03) continue;
				score += n == 1 ? 6 : 2;  // 6p for fundamental, 2p for other harmonics
			}
			if (score > bestScore) {
				bestScore = score;
				bestDiv = div;
			}
		}
		// Construct a Tone by combining the fundamental frequency (freq) and all harmonics
		Tone t;
		std::size_t count = 0;
		double freq = peaks[k].freq / bestDiv;
		t.db = peaks[k].db;
		for (std::size_t n = 1; n <= bestDiv; ++n) {
			// Find the peak for n'th harmonic
			Peak& p = match(peaks, n * freq, freqPerBin);
			if (std::abs(p.freq / n / freq - 1.0) > .03) continue; // Does it match the fundamental freq?
			t.db = std::max(t.db, p.db);
			t.freq += p.freq / n;
			++count;
			t.harmonics[n - 1] = p.db;
			p.clear();
		}
		if (t.db > -40) {
			t.freq /= count;
			t.stabledb = t.db;
			tones.push_back(t);
		}
	}
	mergeWithOld(tones);
	m_tones.swap(tones);
}

void Analyzer::mergeWithOld(tones_t& tones) const {
	tones.sort();
	auto it = tones.begin();
	// Iterate over old tones
	for (auto const& old: m_tones) {
		// Try to find a matching new tone
		while (it != tones.end() && *it < old) ++it;
		// If match found
		if (it != tones.end() && *it == old) {
			// Merge the old tone into the new tone
			it->age = old.age + 1;
			it->stabledb = 0.8 * old.stabledb + 0.2 * it->db;
			it->freq = 0.5 * old.freq + 0.5 * it->freq;
		} else if (old.db > -80.0) {
			// Insert a decayed version of the old tone into new tones
			Tone& t = *tones.insert(it, old);
			t.db -= 5.0;
			t.stabledb -= 0.1;
		}
	}
}

void Analyzer::process() {
	// Try calculating FFT and calculate tones until no more data in input buffer
	while (calcFFT()) calcTones();
}
