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
  stabledb(-80.0),
  age()
{}

void Tone::print() const {
	if (age < Tone::MINAGE) return;
	std::cout << std::fixed << std::setprecision(1) << freq << " Hz, age " << age << ", " << db << " dB:";
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
		double freq, power;
		Peak(double _freq = 0.0, double _power = 0.0): freq(_freq), power(_power) {}
	};
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
	const double minMagnitude = pow(10, -80.0 / 20.0); // -80 dB noise cut
	// Limit frequency range of processing
	const size_t kMin = std::max(size_t(1), size_t(FFT_MINFREQ / freqPerBin));
	const size_t kMax = std::min(FFT_N / 2, size_t(FFT_MAXFREQ / freqPerBin));
	std::vector<Peak> peaks;  // <freq, power>
	for (size_t k = kMin; k <= kMax; ++k) {
		double magnitude = std::abs(m_fft[k]);
		double phase = std::arg(m_fft[k]) / TAU;
		double delta = phase - m_fftLastPhase[k];
		m_fftLastPhase[k] = phase;
		if (magnitude < minMagnitude) continue;
		// Use phase difference over a step to calculate what the frequency must be
		double freq = stepRate * (std::round(k * phaseStep - delta) + delta);
		if (std::abs(freq / freqPerBin - k) <= 1.5) {  // +- 1.5 bins allowed
			peaks.emplace_back(freq, magnitude * magnitude);
		}
	}
	// Group multiple bins' identical frequencies together by total power
	{
		std::sort(peaks.begin(), peaks.end(), [](Peak const& p1, Peak const& p2) {
			return p1.freq < p2.freq;
		});
		std::vector<Peak> p;
		double freqSum = 0.0, powerSum = 0.0;
		for (auto [freq, power] : peaks) {
			if (powerSum > 0.0 && abs(freq - freqSum / powerSum) > 10.0 /* Hz */) {
				p.emplace_back(freqSum / powerSum, powerSum);
				freqSum = powerSum = 0.0;
			}
			freqSum += power * freq;
			powerSum += power;
		}
		if (powerSum > 0.0) p.emplace_back(freqSum / powerSum, powerSum);
		std::sort(p.begin(), p.end(), [](Peak const& p1, Peak const& p2) {
			return p1.power > p2.power;
		});
		if (p.size() > 12) p.resize(12);
		peaks = std::move(p);
	}
	// Combine peaks into tones
	tones_t tones;
	while (!peaks.empty()) {
		// Find the best possible match of any peak being any harmonic
		double bestScore = 0.0;
		double bestFreq = NAN;
		for (Peak const& p : peaks) {
			for (size_t den = 1; den <= 3; ++den) {
				double freq = p.freq / den;
				if (freq < FFT_MINFREQ) break;
				double score = 0.0;
				double freqSum = 0.0;
				for (Peak const& p2: peaks) {
					double f = p2.freq / std::round(p2.freq / freq);
					if (abs(f / freq - 1.0) > 0.06) continue;  // Max. offset +-semitone
					score += p2.power;
					freqSum += p2.power * f;
				}
				score /= 5 + den;
				if (score > bestScore) {
					bestScore = score;
					bestFreq = freqSum / score;
				}
			}
		}
		if (bestScore == 0.0) break;
		// Construct a Tone out of the best match, erasing mathing peaks.
		double power = 0.0;
		peaks.erase(std::remove_if(peaks.begin(), peaks.end(), [&](Peak const& p) {
			double f = p.freq / std::round(p.freq / bestFreq);
			if (abs(f / bestFreq - 1.0) > 0.1) return false;  // +- 1.6 semitones
			power += p.power;
			return true;
		}), peaks.end());
		Tone t;
		t.freq = bestFreq;
		t.db = 10.0 * std::log10(power);
		double min = tones.empty() ? -50.0 : std::max(-50.0, tones.front().db - 20.0);
		if (t.db < min) break;
		tones.push_back(t);
	}
	// Logging for debugging purposes
	if (!tones.empty()) {
		std::ostringstream oss;
		for (auto t : tones) {
			oss << " " << std::round(t.freq) << "Hz/" << std::round(t.db) << "dB";
		}
		std::clog << "pitch/debug: Tones"  + oss.str() + "\n" << std::flush;
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
