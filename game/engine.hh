#pragma once
#ifndef PERFORMOUS_ENGINE_HH
#define PERFORMOUS_ENGINE_HH

#include "audio.hh"
#include "color.hh"
#include "pitch.hh"
#include "screen.hh"
#include "songs.hh"
#include "util.hh"
#include "xtime.hh"
#include "configuration.hh"
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <limits>
#include <list>
#include <utility>

/// player class
struct Player {
	/// currently playing song
	Song& m_song;
	/// sound analyzer
	Analyzer& m_analyzer;
	/// player color for bars, waves, scores
	Color m_color;
	/// typedef for pitch
	typedef std::vector<std::pair<double, double> > pitch_t;
	/// player's pitch
	pitch_t m_pitch;
	/// current position in pitch vector (first unused spot)
	size_t m_pos;
	/// score for current song
	double m_score;
	/// activity timer
	unsigned m_activitytimer;
	/// score iterator
	Notes::const_iterator m_scoreIt;
	/// constructor
	Player(Song& song, Analyzer& analyzer, size_t frames): m_song(song), m_analyzer(analyzer), m_pitch(frames, std::make_pair(getNaN(), -getInf())), m_pos(), m_score(), m_activitytimer(), m_scoreIt(m_song.notes.begin()) {}
	/// prepares analyzer
	void prepare() { m_analyzer.process(); }
	/// updates player stats
	void update();
	/// player activity singing
	float activity() const { return m_activitytimer / 300.0; }
	/// get player's score
	int getScore() const {
		return 10000.0 * m_score;
	}

};

namespace {
	const Color playerColors[] = {
		Color(0.2, 0.5, 0.7),
		Color(0.8, 0.3, 0.3),
		Color(0.2, 0.9, 0.2),
		Color(1.0, 0.6, 0.0)
	};
	size_t playerColorsSize = sizeof(playerColors) / sizeof(*playerColors);
}

/// performous engine
class Engine {
	Audio& m_audio;
	Song& m_song;
	std::list<Player> m_players;
	volatile double m_latencyAR;  // Audio roundtrip latency (don't confuse with latencyAV)
	size_t m_time;
	volatile bool m_quit;
	boost::scoped_ptr<boost::thread> m_thread;

  public:
	/// timestepping constant
	static const double TIMESTEP;
	/** Construct a new Engine with the players that go with it.
	* The engine runs in the background in a separate thread.
	* @param audio A reference will be stored in order to monitor playback time.
	* @param anBegin Analyzers to use (beginning iterator)
	* @param anEnd Analyzers to use (ending iterator)
	* @param song Song to play
	**/
	template <typename FwdIt> Engine(Audio& audio, Song& song, FwdIt anBegin, FwdIt anEnd):
	  m_audio(audio), m_song(song), m_latencyAR(config["audio/round-trip"].get_f()), m_time(), m_quit()
	{
		size_t frames = m_audio.getLength() / Engine::TIMESTEP;
		while (anBegin != anEnd) m_players.push_back(Player(song, *anBegin++, frames));
		size_t player = 0;
		for (std::list<Player>::iterator it = m_players.begin(); it != m_players.end(); ++it, ++player) it->m_color = playerColors[player % playerColorsSize];
		m_thread.reset(new boost::thread(boost::ref(*this)));
	}
	~Engine() { m_quit = true; m_thread->join(); }
	/// kills playback
	void kill() { m_quit = true; }
	/// sets latency
	void setLatencyAR(double value) { m_latencyAR = config["audio/round-trip"].f() = clamp(round(value * 1000.0) / 1000.0, 0.0, 0.5); }
	/// get latency
	double getLatencyAR() const { return m_latencyAR; }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()() {
		while (!m_quit) {
			std::for_each(m_players.begin(), m_players.end(), boost::bind(&Player::prepare, _1));
			double t = m_audio.getPosition() - m_latencyAR;
			double timeLeft = m_time * TIMESTEP - t;
			if (timeLeft > 0.0) { boost::thread::sleep(now() + std::min(TIMESTEP, timeLeft)); continue; }
			for (Notes::const_iterator it = m_song.notes.begin(); it != m_song.notes.end(); ++it) it->power = 0.0f;
			std::for_each(m_players.begin(), m_players.end(), boost::bind(&Player::update, _1));
			++m_time;
		}
	}
	/// gets list of players currently plugged in
	std::list<Player> const& getPlayers() const {
		// XXX: Technically this code is incorrect because it returns a reference to a structure that is being at the same modified by another thread (and nothing's even marked volatile). This is done in order to improve performance.
		return m_players;
	}
};

#endif
