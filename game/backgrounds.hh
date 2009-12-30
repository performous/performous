#pragma once

#include "animvalue.hh"
#include "fs.hh"
#include "song.hh"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <set>
#include <vector>

/// songs class for songs screen
class Backgrounds: boost::noncopyable {
  public:
	/// constructor
	Backgrounds(): m_bgiter(0), m_dirty(false), m_loading(false)
	{
		reload();
	}
	~Backgrounds() {
		m_loading = false; // Terminate loading if currently in progress
		m_thread->join();
	}
	/// reloads backgrounds list
	void reload();
	/// array access
	std::string& operator[](std::size_t pos) { return m_bgs.at(pos); }
	/// number of backgrounds
	int size() const { return m_bgs.size(); };
	/// true if empty
	int empty() const { return m_bgs.empty(); };
	/// returns random background
	std::string getRandom();
	
  private:
	typedef std::vector<std::string> BGVector;
	BGVector m_bgs;
	int m_bgiter;
	void reload_internal();
	void reload_internal(fs::path const& p);
	volatile bool m_dirty;
	volatile bool m_loading;
	boost::scoped_ptr<boost::thread> m_thread;
	mutable boost::mutex m_mutex;
};

