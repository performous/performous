#include "audio.hh"
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

struct Color { float r, g, b; };

struct Player {
	Analyzer& m_analyzer;
	Color m_color;
	typedef std::vector<std::pair<double, double> > pitch_t;
	pitch_t m_pitch;
	double m_score;
	Song::notes_t::const_iterator m_scoreIt;
	Player(Analyzer& analyzer): m_analyzer(analyzer), m_score() {}
	void prepare() { m_analyzer.process(); }
	void update() {
		Tone const* t = m_analyzer.findTone();
		if (t) {
			Song const& s = CScreenManager::getSingletonPtr()->getSongs()->current(); // XXX: Kill ScreenManager
			m_scoreIt = s.notes.begin(); // TODO: optimize
			m_pitch.push_back(std::make_pair(t->freq, t->stabledb));
			double beginTime = 0.01 * (m_pitch.size() - 1);  // XXX: 0.01 = Engine::TIMESTEP
			double endTime = beginTime + 0.01;
			while (m_scoreIt != s.notes.end()) {
				m_score += s.m_scoreFactor * m_scoreIt->score(s.scale.getNote(t->freq), beginTime, endTime);
				if (endTime < m_scoreIt->end) break;
				++m_scoreIt;
			}
			m_score = std::min(1.0, std::max(0.0, m_score));
		}
		else m_pitch.push_back(std::make_pair(std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::infinity()));
	}
	int getScore() const {
		return 10000.0 * m_score;
	}

};

namespace {
	Color playerColors[] = {
		{ 52/255.0, 101/255.0, 164/255.0 },
		{ 0.8, 0.3, 0.3 }
	};
}

class Engine {
	CAudio& m_audio;
	std::list<Player> m_players;
	size_t m_time;
	volatile bool m_quit;
	boost::mutex m_mutex;
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
	  m_audio(audio), m_players(anBegin, anEnd), m_time(), m_quit(), m_thread(boost::ref(*this))
	{
		size_t player = 0;
		for (std::list<Player>::iterator it = m_players.begin(); it != m_players.end(); ++it, ++player) it->m_color = playerColors[player]; // FIXME: don't segfault with more than two players
	}
	~Engine() { m_quit = true; m_thread.join(); }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()() {
		while (!m_quit) {
			std::for_each(m_players.begin(), m_players.end(), boost::bind(&Player::prepare, _1));
			double t = m_audio.getPosition() - 0.05; // Compensate avg. audio input lag
			double timeLeft = m_time * TIMESTEP - t;
			if (timeLeft > 0.0) { boost::thread::sleep(now() + std::min(TIMESTEP, timeLeft * 0.1)); continue; }
			boost::mutex::scoped_lock l(m_mutex);
			std::for_each(m_players.begin(), m_players.end(), boost::bind(&Player::update, _1));
			++m_time;
		}
	}
	std::list<Player> getPlayers() {
		boost::mutex::scoped_lock l(m_mutex);
		return m_players;
	}
};

