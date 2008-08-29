#ifndef USNG_PITCH_H_INCLUDED
#define USNG_PITCH_H_INCLUDED

#include <boost/thread/mutex.hpp>
#include <complex>
#include <deque>
#include <list>
#include <vector>
#include <algorithm>

struct Tone {
	static const std::size_t MAXHARM = 48;
	static const std::size_t MINAGE = 2;
	double freq;
	double db;
	double stabledb;
	double harmonics[MAXHARM];
	std::size_t age;
	Tone();
	void print() const;
	bool operator==(double f) const;
	void update(Tone const& t);
	static bool dbCompare(Tone const& l, Tone const& r) { return l.db < r.db; }
};

static inline bool operator==(Tone const& lhs, Tone const& rhs) { return lhs == rhs.freq; }
static inline bool operator!=(Tone const& lhs, Tone const& rhs) { return !(lhs == rhs); }
static inline bool operator<=(Tone const& lhs, Tone const& rhs) { return lhs.freq < rhs.freq || lhs == rhs; }
static inline bool operator>=(Tone const& lhs, Tone const& rhs) { return lhs.freq > rhs.freq || lhs == rhs; }
static inline bool operator<(Tone const& lhs, Tone const& rhs) { return lhs.freq < rhs.freq && lhs != rhs; }
static inline bool operator>(Tone const& lhs, Tone const& rhs) { return lhs.freq > rhs.freq && lhs != rhs; }

class Analyzer {
  public:
	typedef std::vector<std::complex<float> > fft_t;
	typedef std::list<Tone> tones_t;
	Analyzer(std::size_t step = 500);
	void setRate(double rate) { m_rate = rate; }
	/** Add input data to buffer. This is thread-safe (against other functions). **/
	template <typename InIt> void input(InIt begin, InIt end) {
		boost::mutex::scoped_lock l(m_mutex);
		std::copy(begin, end, std::back_inserter(m_buf));
		// Avoid running out of memory even if process() never gets called
		if (m_buf.size() > 10000) m_buf.erase(m_buf.begin(), m_buf.begin() + 10000);
	}
	/** Call this to process all data input so far. **/
	void process();
	/** Get the raw FFT. **/
	fft_t const& getFFT() const { return m_fft; }
	/** Get the peak level in dB (negative value, 0.0 = clipping). **/
	double getPeak() const { return m_peak; }
	/** Get a list of all tones detected. **/
	tones_t const& getTones() const { return m_tones; }
	/** Find a tone within the singing range; prefers strong tones around 250 Hz. **/
	Tone const* findTone(double minfreq = 70.0, double maxfreq = 600.0) const {
		if (m_tones.empty()) return NULL;
		double db = std::max_element(m_tones.begin(), m_tones.end(), Tone::dbCompare)->db;
		Tone const* best = NULL;
		double bestscore = 0;
		for (tones_t::const_iterator it = m_tones.begin(); it != m_tones.end(); ++it) {
			if (it->db < db - 20.0 || it->freq < minfreq || it->age < Tone::MINAGE) continue;
			if (it->freq > maxfreq) break;
			double score = it->db - std::abs(it->freq - 250.0) / 10.0;
			if (best && bestscore > score) break;
			best = &*it;
			bestscore = score;
		}
		return best;
	}
  private:
	std::size_t m_step;
	double m_rate;
	std::vector<float> m_window;
	mutable boost::mutex m_mutex;
	std::deque<float> m_buf;
	fft_t m_fft;
	std::vector<float> m_fftLastPhase;
	double m_peak;
	tones_t m_tones;
	bool calcFFT();
	void calcTones();
	void mergeWithOld(tones_t& tones) const;
};

/*
class Player {
	Song& m_song;
	Analyzer& m_analyzer;
  public:
	Player(Song& song, Analyzer& analyzer): m_song(song), m_analyzer(analyzer) {}
	void process(double time) {
		CScreenManager* sm = CScreenManager::getSingletonPtr();
		Song& song = sm->getSongs()->current();
		m_analyzer.process();
	}
	double score() const {
		return 0;
	}
};

class GameEngine {
	typedef std::vector<Player> players_t;
	players_t m_players;
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_quit;
  public:
	GameEngine(): m_quit() {
		m_thread.reset(new boost::thread(boost::ref(*this)));
	}
	~GameEngine() {
		m_quit = true;
		m_thread->join();
	}
	operator()() {
		while (!m_quit) {
			CScreenManager* sm = CScreenManager::getSingletonPtr();
			double time = sm->getAudio()->getTime();
			for (players_t::iterator it = m_players.begin(); it != m_players.end(); ++it) {
				it->process(time);
			}
		}
	}
};
*/

#endif

