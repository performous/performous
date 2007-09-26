#ifndef __RECORD_H_
#define __RECORD_H_

#include "/root/audio-test/audio.hpp"
#include "../config.h"
#include <deque>
#include <vector>

static const unsigned int DEFAULT_RATE = 48000;

struct Peak {
	double freq;
	double db;
	Peak(double freq = 0.0, double db = -std::numeric_limits<double>::infinity()): freq(freq), db(db) {}
};

class Tone {
	double m_freqSum; // Sum of the fundamental frequencies of all harmonics
	unsigned int m_harmonics;
	unsigned int m_hEven;
	unsigned int m_hHighest;
	double m_dbHighest;
	unsigned int m_dbHighestH;
  public:
	Tone();
	void print() const;
	void combine(Peak& p, unsigned int h);
	bool isWeak() const;
	double db() const { return m_dbHighest; }
	double freq() const { return m_freqSum / m_harmonics; }
	bool operator==(double f) const;
	Tone& operator+=(Tone const& t);
};

static inline bool operator==(Tone const& lhs, Tone const& rhs) { return lhs == rhs.freq(); }
static inline bool operator!=(Tone const& lhs, Tone const& rhs) { return !(lhs == rhs); }
static inline bool operator<=(Tone const& lhs, Tone const& rhs) { return lhs.freq() < rhs.freq() || lhs == rhs; }
static inline bool operator>=(Tone const& lhs, Tone const& rhs) { return lhs.freq() > rhs.freq() || lhs == rhs; }
static inline bool operator<(Tone const& lhs, Tone const& rhs) { return lhs.freq() < rhs.freq() && lhs != rhs; }
static inline bool operator>(Tone const& lhs, Tone const& rhs) { return lhs.freq() > rhs.freq() && lhs != rhs; }

class Record;

/** @short A wrapper for SDL mutex. **/
class Mutex {
	SDL_mutex* mutex;
	Mutex(Mutex const&); // Prevent copying
	Mutex& operator=(Mutex const&); // ... and assignment
  public:
	Mutex(): mutex(SDL_CreateMutex()) {}
	~Mutex() { SDL_DestroyMutex(mutex); }
	void lock() { SDL_mutexP(mutex); }
	void unlock() { SDL_mutexV(mutex); }
};

/** @short A scoped RAII wrapper for mutex locking. **/
class ScopedLock {
	Mutex& mutex;
	ScopedLock(ScopedLock const&); // Prevent copying
	ScopedLock& operator=(ScopedLock const&); // ... and assignment
  public:
	ScopedLock(Mutex& mutex): mutex(mutex) { mutex.lock(); }
	~ScopedLock() { mutex.unlock(); }
};

class CFft {
  public:
	CFft(size_t fftSize = 4096, size_t fftStep = 1500);
	~CFft();
	void operator()(audio::pcm_data& data, audio::settings const& s);
	/** Get the peak level in dB (negative value, 0.0 = clipping). **/
	double getPeak() const { return m_peak; }
	/** Get the primary (singing) frequency. **/
	double getFreq() const { return m_freq; }
	/** Get a list of all tones detected. **/
	std::vector<Tone> getTones() const {
		ScopedLock l(m_mutex);
		return m_tones;
	}
  private:
	mutable Mutex m_mutex;
	size_t fftSize;
	size_t fftStep;
	float *fftIn;
	fftwf_complex *fftOut;
	fftwf_plan fftPlan;
	std::vector<float> fftLastPhase;
	std::vector<float> window;
	volatile double m_peak;
	volatile double m_freq;
	std::deque<float> sampleBuffer;
	std::vector<Tone> m_tones; // Synchronized access only!
	std::vector<Tone> m_oldTones;
};

class Capture {
	CFft m_fft;
	audio::settings m_rs;
	audio::record m_record;
  public:
	Capture(std::string const& device = "", std::size_t rate = DEFAULT_RATE):
	  m_rs(audio::settings(device)
	  .set_callback(boost::ref(m_fft))
	  .set_channels(1)
	  .set_rate(rate)
	  .set_debug(std::cerr)),
	  m_record(m_rs)
	{}
	~Capture() {}
	CFft& fft() { return m_fft; }
};

class MusicalScale {
	double baseFreq;
	static const int baseId = 33;
  public:
	MusicalScale(double baseFreq = 440.0): baseFreq(baseFreq) {}
	std::string getNoteStr(double freq) const;
	double getNoteFreq(int id) const;
	int getNoteId(double freq) const;
	double getNoteOffset(double freq) const;
};

#endif
