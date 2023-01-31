#pragma once

#include "animvalue.hh"
#include "fs.hh"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <vector>
#include "screen.hh"
#include "songorder.hh"
#include <shared_mutex>

class Game;
class Song;
class Database;

/// songs class for songs screen
class Songs {
  public:
	Songs(const Songs&) = delete;
	const Songs& operator=(const Songs&) = delete;
	/// constructor
	Songs(Database& database, std::string const& songlist = std::string());
	~Songs();
	/// updates filtered songlist
	void update();
	/// reloads songlist
	void reload();
	/// array access
	std::shared_ptr<Song> operator[](std::size_t pos) { return m_filtered[pos]; }
	/// number of songs
	size_t size() const { return m_filtered.size(); }
	/// true if empty
	bool empty() const { return m_filtered.empty(); }
	/// advances to next song
	void advance(int diff) {
		std::ptrdiff_t size = static_cast<int>(m_filtered.size());
		if (size == 0) return;  // Do nothing if no songs are available
		std::ptrdiff_t _current = (math_cover.getTarget() + diff) % size;
		if (_current < 0) _current += size;
		math_cover.setTarget(_current, size);
	}
	/// get current id
	std::ptrdiff_t currentId() const { return math_cover.getTarget(); }
	/// gets current position
	double currentPosition() { return math_cover.getValue(); }
	/// gets current velocity
	double currentVelocity() const { return math_cover.getVelocity(); }
	/// sets margins for animation
	void setAnimMargins(double left, double right) { math_cover.setMargins(left, right); }
	/// @return current song
	std::shared_ptr<Song> currentPtr() const;
	/// @return current song
	Song& current();
	/// @return current Song
	Song const& current() const;
	/// filters songlist by regular expression
	void setFilter(std::string const& regex);
	/// Get the current song type filter number
	unsigned short typeNum() const { return m_type; }
	/// Description of the current song type filter
	std::string typeDesc() const;
	enum class SortChange : int { BACK = -1, RESET = 0, FORWARD = 1};
	/// Change song type filter (diff is normally -1 or 1; 0 has special meaning of reset)
	void typeChange(SortChange diff);
	/// Cycle song type filters by filter category (0 = none, 1..4 = different categories)
	void typeCycle(unsigned short cat);
	unsigned short sortNum() const { return m_order; }
	/// Description of the current sort mode
	std::string getSortDescription() const;
	/// Change sorting mode (diff is normally -1 or 1)
	void sortChange(Game&, SortChange diff);
	void sortSpecificChange(unsigned short sortOrder, bool descending = false);
	/// parses file into Song &tmp
	void parseFile(Song& tmp);
	std::atomic<bool> doneLoading{ false };
	std::atomic<bool> displayedAlert{ false };
	size_t loadedSongs() const { std::shared_lock<std::shared_mutex> l(m_mutex); return m_songs.size(); }
	void addSongOrder(SongOrderPtr);

  private:
	void LoadCache();
	void CacheSonglist();

	class RestoreSel;
	std::string m_songlist;
	// Careful the m_songs needs to be correctly locked when accessed, and
	// especially, the reload_internal thread expects to be the only thread
	// to modify this member (any other thread may read it).
	SongCollection m_songs, m_filtered;
	AnimValue m_updateTimer;
	AnimAcceleration math_cover;
	std::string m_filter;
	Database & m_database;
	unsigned short m_type = 0;
	unsigned short m_order;  // Set by constructor
	void dumpSongs_internal() const;
	void reload_internal();
	void reload_internal(fs::path const& p);
	void randomize_internal();
	void filter_internal();
	void sort_internal(bool descending = false);
	std::atomic<bool> m_dirty{ false };
	std::atomic<bool> m_loading{ false };
	std::unique_ptr<std::thread> m_thread;
	mutable std::shared_mutex m_mutex;
	std::vector<SongOrderPtr> m_songOrders;
};
