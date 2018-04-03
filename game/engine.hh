#pragma once

#include <boost/thread/thread.hpp>
#include <memory>

class Audio;
class Database;
class VocalTrack;

/// performous engine
class Engine {
	Audio& m_audio;
	double m_time;
	volatile bool m_quit;
	Database& m_database;
	std::unique_ptr<boost::thread> m_thread;

  public:
	typedef std::vector<VocalTrack*> VocalTrackPtrs;
	static const double TIMESTEP;  ///< The duration of one engine time step in seconds
	/// Construct an engine thread with vocal tracks and players specified by parameters
	Engine(Audio& audio, VocalTrackPtrs vocals, Database& database);
	~Engine() { kill(); }
	/// Terminates processing
	void kill() { m_quit = true; m_thread->join(); }
	/** Used internally for boost::thread. Do not call this yourself. (boost::thread requires this to be public). **/
	void operator()();
};
