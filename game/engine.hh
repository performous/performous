#pragma once

#include "audio.hh"
#include "color.hh"
#include "song.hh"
#include "configuration.hh"
#include "database.hh"
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <list>

namespace {
	const Color playerColors[] = {
		Color(0.125, 1.0, 0.1328125),
		Color(0.12890625, 0.546875, 1.0),
		Color(1.0, 0.12890625, 0.98046875),
		Color(1.0, 0.578125, 0.12890625)
	};
	size_t playerColorsSize = sizeof(playerColors) / sizeof(*playerColors);
}

#include <iostream>

/// performous engine
class Engine {
	Audio& m_audio;
	Song& m_song;
	size_t m_time;
	volatile bool m_quit;
	Database& m_database;
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
	template <typename FwdIt> Engine(Audio& audio, Song& song, FwdIt anBegin, FwdIt anEnd, Database& database):
	  m_audio(audio), m_song(song), m_time(), m_quit(), m_database(database)
	{
		// clear old player information
		m_database.cur.clear();
		m_database.scores.clear();
		// Only add players if the vocal track has sensible length (not NaN or extremely long)
		std::cout << "Endtime: " << song.endTime << std::endl;
		if (song.endTime < 10000.0) {
			// Calculate the space required for pitch frames
			size_t frames = song.endTime / Engine::TIMESTEP;
			while (anBegin != anEnd) m_database.cur.push_back(Player(song, *anBegin++, frames));
			size_t player = 0;
			for (std::list<Player>::iterator it = m_database.cur.begin(); it != m_database.cur.end(); ++it, ++player) it->m_color = playerColors[player % playerColorsSize];
		}
		m_thread.reset(new boost::thread(boost::ref(*this)));
	}
	~Engine() { kill(); }
	/// Terminates processing
	void kill() { m_quit = true; m_thread->join(); }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()() {
		while (!m_quit) {
			std::for_each(m_database.cur.begin(), m_database.cur.end(), boost::bind(&Player::prepare, _1));
			double t = m_audio.getPosition() - config["audio/round-trip"].f();
			double timeLeft = m_time * TIMESTEP - t;
			if (timeLeft > 0.0) { boost::thread::sleep(now() + std::min(TIMESTEP, timeLeft)); continue; }
			for (Notes::const_iterator it = m_song.notes.begin(); it != m_song.notes.end(); ++it) it->power = 0.0f;
			std::for_each(m_database.cur.begin(), m_database.cur.end(), boost::bind(&Player::update, _1));
			++m_time;
		}
	}
};
