#pragma once
#ifndef PERFORMOUS_ENGINE_HH
#define PERFORMOUS_ENGINE_HH

#include "audio.hh"
#include "color.hh"
#include "pitch.hh"
#include "songs.hh"
#include "xtime.hh"
#include "configuration.hh"
#include "player.hh"
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <list>

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
	  m_audio(audio), m_song(song), m_time(), m_quit()
	{
		size_t frames = m_audio.getLength() / Engine::TIMESTEP;
		while (anBegin != anEnd) m_players.push_back(Player(song, *anBegin++, frames));
		size_t player = 0;
		for (std::list<Player>::iterator it = m_players.begin(); it != m_players.end(); ++it, ++player) it->m_color = playerColors[player % playerColorsSize];
		m_thread.reset(new boost::thread(boost::ref(*this)));
	}
	~Engine() { kill(); m_thread->join(); }
	/// Terminates processing
	void kill() { m_quit = true; }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()() {
		while (!m_quit) {
			std::for_each(m_players.begin(), m_players.end(), boost::bind(&Player::prepare, _1));
			double t = m_audio.getPosition() - config["audio/round-trip"].f();
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
