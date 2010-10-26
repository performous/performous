#pragma once

#include "audio.hh"
#include "song.hh"
#include "configuration.hh"
#include "database.hh"
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <list>
#include <iostream>

/// performous engine
class Engine {
	Audio& m_audio;
	VocalTrack& m_vocal;
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
	* @param vocal Song to play
	**/
	template <typename FwdIt> Engine(Audio& audio, VocalTrack& vocal, FwdIt anBegin, FwdIt anEnd, Database& database):
	  m_audio(audio), m_vocal(vocal), m_time(), m_quit(), m_database(database)
	{
		// clear old player information
		m_database.cur.clear();
		m_database.scores.clear();
		// Only add players if the vocal track has sensible length (not NaN or extremely long)
		if (vocal.endTime < 10000.0) {
			// Calculate the space required for pitch frames
			size_t frames = vocal.endTime / Engine::TIMESTEP;
			while (anBegin != anEnd) m_database.cur.push_back(Player(vocal, *anBegin++, frames));
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
			if (timeLeft != timeLeft || timeLeft > 1.0) timeLeft = 1.0;  // FIXME: Workaround for NaN values and other weirdness (should fix the weirdness instead)
			if (timeLeft > 0.0) { boost::thread::sleep(now() + std::min(TIMESTEP, timeLeft)); continue; }
			for (Notes::const_iterator it = m_vocal.notes.begin(); it != m_vocal.notes.end(); ++it) it->power = 0.0f;
			std::for_each(m_database.cur.begin(), m_database.cur.end(), boost::bind(&Player::update, _1));
			++m_time;
		}
	}
};
