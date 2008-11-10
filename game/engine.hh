#include "audio.hh"
#include "color.hh"
#include "pitch.hh"
#include "xtime.hh"
#include "songs.hh"
#include "screen.hh"
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <limits>
#include <list>
#include <utility>

struct Player {
	Analyzer& m_analyzer;
	Color m_color;
	typedef std::vector<std::pair<double, double> > pitch_t;
	pitch_t m_pitch;
	double m_score;
	unsigned m_activitytimer;
	Song::notes_t::const_iterator m_scoreIt;
	Player(Analyzer& analyzer): m_analyzer(analyzer), m_score(), m_activitytimer() {}
	void prepare() { m_analyzer.process(); }
	void update();
	float activity() const { return m_activitytimer / 300.0; }
	int getScore() const {
		return 10000.0 * m_score;
	}

};

namespace {
	const Color playerColors[] = {
		Color(52/255.0, 101/255.0, 164/255.0),
		Color(0.8, 0.3, 0.3),
		Color(0.3, 1.0, 0.3),
		Color(1.0, 0.6, 0.0)
	};
	size_t playerColorsSize = sizeof(playerColors) / sizeof(*playerColors);
}

class Engine {
	CAudio& m_audio;
	std::list<Player> m_players;
	volatile double m_latencyAR;  // Audio roundtrip latency (don't confuse with latencyAV)
	size_t m_time;
	volatile bool m_quit;
	mutable boost::mutex m_mutex;
	boost::thread m_thread;  // IMPORTANT: This must be the last variable
  public:
	static const double TIMESTEP;
	/** Construct a new Engine with the players that go with it.
	* The engine runs in the background in a separate thread.
	* @param audio A reference will be stored in order to monitor playback time.
	* @param anBegin Analyzers to use (beginning iterator)
	* @param anBegin Analyzers to use (ending iterator)
	**/
	template <typename FwdIt> Engine(CAudio& audio, FwdIt anBegin, FwdIt anEnd):
	  m_audio(audio), m_players(anBegin, anEnd), m_latencyAR(0.05), m_time(), m_quit(), m_thread(boost::ref(*this))
	{
		size_t player = 0;
		for (std::list<Player>::iterator it = m_players.begin(); it != m_players.end(); ++it, ++player) it->m_color = playerColors[player % playerColorsSize];
	}
	~Engine() { m_quit = true; m_thread.join(); }
	void setLatencyAR(double value) { m_latencyAR = std::min(std::max(0.0, value), 0.5); }
	double getLatencyAR() const { return m_latencyAR; }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()() {
		while (!m_quit) {
			std::for_each(m_players.begin(), m_players.end(), boost::bind(&Player::prepare, _1));
			double t = m_audio.getPosition() - m_latencyAR;
			double timeLeft = m_time * TIMESTEP - t;
			if (timeLeft > 0.0) { boost::thread::sleep(now() + std::min(TIMESTEP, timeLeft)); continue; }
			boost::mutex::scoped_lock l(m_mutex);
			Song::notes_t const& n = CScreenManager::getSingletonPtr()->getSongs()->current().notes; // TODO: Kill ScreenManager
			for (Song::notes_t::const_iterator it = n.begin(); it != n.end(); ++it) it->power = 0.0f;
			std::for_each(m_players.begin(), m_players.end(), boost::bind(&Player::update, _1));
			++m_time;
		}
	}
	std::list<Player> getPlayers() const {
		boost::thread::yield(); // Try to let engine perform its run right before getting the data
		boost::mutex::scoped_lock l(m_mutex);
		return m_players;
	}
};

