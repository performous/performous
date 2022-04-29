#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

class Audio;
class Database;
class VocalTrack;

/// performous engine
class Engine {
	Audio& m_audio;
	float m_time;
	std::atomic<bool> m_quit{ false };
	Database& m_database;
	std::unique_ptr<std::thread> m_thread;

  public:
	typedef std::vector<VocalTrack*> VocalTrackPtrs;
	static const float TIMESTEP;  ///< The duration of one engine time step in seconds
	/// Construct an engine thread with vocal tracks and players specified by parameters
	Engine(Audio& audio, VocalTrackPtrs vocals, Database& database);
	~Engine() { kill(); }
	/// Terminates processing
	void kill() { 
		m_quit = true;
		if (m_thread->joinable()) m_thread->join();
		}
	/** Used internally for std::thread. Do not call this yourself. (std::thread requires this to be public). **/
	void operator()();
};
