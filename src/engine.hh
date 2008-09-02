#include "audio.hh"
#include "pitch.hh"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <list>

struct Player {
	Analyzer const& m_analyzer;
	std::list<std::pair<double, double> > m_pitch;
	double m_score;
	Player(Analyzer& analyzer): m_analyzer(analyzer), m_score() {}
	void prepare() { m_analyzer.process(); }
	void update(unsigned time) {
	}
	double score() const {
		return m_score;
	}

};

namespace {
	const double ENGINE_TIMESTEP = 0.01; // ms
}

class Engine {
	CAudio& m_audio;
	std::list<Player> m_players
	size_t m_time;
	volatile bool m_quit;
	boost::mutex m_mutex;
	boost::thread m_thread;  // IMPORTANT: This must be the last variable
  public:
	/** Construct a new Engine with the players that go with it.
	* The engine runs in the background in a separate thread.
	* @param audio A reference will be stored in order to monitor playback time.
	* @param anBegin Analyzers to use (beginning iterator)
	* @param anBegin Analyzers to use (ending iterator)
	**/
	template <typename FwdIt> Engine(CAudio& audio, FwdIt anBegin, FwdIt anEnd):
	  m_audio(audio), m_players(anBegin, anEnd), m_time(), m_quit(), m_thread(boost::ref(*this)) {}
	~Engine() { m_quit = true; m_thread.join(); }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()() {
		while (!m_quit) {
			std::for_each(m_players.begin(), m_players.end(), boost::bind(prepare, m_time));
			double t = m_audio.getPosition();
			double timeLeft = m_time * ENGINE_TIMESTEP - t;
			if (timeLeft > 0.0) { boost::thread::sleep(now() + timeLeft * 0.6); continue; }
			boost::mutex::scoped_lock l(m_mutex);
			std::for_each(m_players.begin(), m_players.end(), boost::bind(update, m_time));
			++m_time;
		}
	}
	std::list<Player> getPlayers() {
		boost::mutex::scoped_lock l(m_mutex);
		return m_players;
	}
};

