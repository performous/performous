#pragma once

#include "fs.hh"
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/// songs class for songs screen
class Backgrounds {
  public:
	/// constructor
	Backgrounds(const Backgrounds&) = delete;
  	const Backgrounds& operator=(const Backgrounds&) = delete;
	Backgrounds() {
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
	size_t size() const { return m_bgs.size(); };
	/// true if empty
	bool empty() const { return m_bgs.empty(); };
	/// returns random background
	std::string getRandom();

  private:
	typedef std::vector<std::string> BGVector;
	BGVector m_bgs;
	unsigned m_bgiter = 0;
	void reload_internal();
	void reload_internal(fs::path const& p);
	std::atomic<bool> m_dirty{ false };
	std::atomic<bool> m_loading{ false };
	std::unique_ptr<std::thread> m_thread;
	mutable std::mutex m_mutex;
};

