#pragma once

#include "song.hh"
#include "database.hh"
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <list>
#include <iostream>

class Audio;

/// performous engine
class Engine {
	Audio& m_audio;
	size_t m_time;
	volatile bool m_quit;
	Database& m_database;
	boost::scoped_ptr<boost::thread> m_thread;

  public:
	typedef std::vector<VocalTrack*> VocalTrackPtrs;
	/// timestepping constant
	static const double TIMESTEP;
	/** Construct a new Engine with the players that go with it.
	* The engine runs in the background in a separate thread.
	* @param audio A reference will be stored in order to monitor playback time.
	* @param anBegin Analyzers to use (beginning iterator)
	* @param anEnd Analyzers to use (ending iterator)
	* @param vocal Song to play
	**/
	template <typename FwdIt> Engine(Audio& audio, VocalTrackPtrs vocals, FwdIt anBegin, FwdIt anEnd, Database& database):
	  m_audio(audio), m_time(), m_quit(), m_database(database)
	{
		if (vocals.empty()) throw std::runtime_error("Engine needs at least one vocal track");
		// Remove unsensibly long tracks (also NaN)
		for (VocalTrackPtrs::iterator it = vocals.begin(); it != vocals.end(); ) {
			if ((*it) && (*it)->endTime < 10000.0) ++it; else it = vocals.erase(it);
		}
		// Clear old player information
		m_database.cur.clear();
		m_database.scores.clear();
		size_t i = 0;
		while (anBegin != anEnd && !vocals.empty()) {
			// Calculate the space required for pitch frames
			size_t frames = vocals[i]->endTime / Engine::TIMESTEP;
			m_database.cur.push_back(Player(*vocals[i], *anBegin++, frames));
			i = (i+1) % vocals.size();
		}
		m_thread.reset(new boost::thread(boost::ref(*this)));
	}
	~Engine() { kill(); }
	/// Terminates processing
	void kill() { m_quit = true; m_thread->join(); }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()();
};
