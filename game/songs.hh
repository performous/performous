#pragma once

#include "animvalue.hh"
#include "fs.hh"
#include "screen.hh"
#include "songorder.hh"
#include "utils/cycle.hh"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include <vector>

class Game;
class Song;
class Database;

using SongPtr = std::shared_ptr<Song>;
using SongCollection = std::vector<SongPtr>;


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
	SongCollection& getSongs(bool webServer = false) { std::shared_lock<std::shared_mutex> l(m_mutex); return (webServer ? m_webServerFiltered : m_filtered); }
	SongCollection const& getSongs(bool webServer = false) const { std::shared_lock<std::shared_mutex> l(m_mutex); return (webServer ? m_webServerFiltered : m_filtered); }
	/// number of songs
	std::size_t size(bool webServer = false) const { return getSongs(webServer).size(); }
	/// true if empty
	bool empty(bool webServer = false) const { return getSongs(webServer).empty(); }
	/// advances to previous/next song
	void advance(int diff, bool webServer = false);
	/// get current id
	std::ptrdiff_t currentId() const { return math_cover.getTarget(); }
	/// gets current position
	double currentPosition() { return math_cover.getValue(); }
	/// gets current velocity
	double currentVelocity() const { return math_cover.getVelocity(); }
	/// sets margins for animation
	void setAnimMargins(double left, double right) { math_cover.setMargins(left, right); }
	/// @return current song
	std::shared_ptr<Song> currentPtr(bool webServer = false) const;
	/// @return current song
	Song& current(bool webServer = false);
	/// @return current Song
	Song const& current(bool webServer = false) const;
	/// filters songlist by regular expression
	void setFilter(std::string const& regex, bool webServer = false);
	/// Get the current song type filter number
	unsigned short typeNum() const { return m_type; }
	/// Description of the current song type filter
	std::string typeDesc() const;
	enum class SortChange : int { BACK = -1, RESET = 0, FORWARD = 1};
	/// Change song type filter (diff is normally -1 or 1; 0 has special meaning of reset)
	void typeChange(SortChange diff, bool webServer = false);
	/// Cycle song type filters by filter category (0 = none, 1..4 = different categories)
	void typeCycle(unsigned short cat, bool webServer = false);
	Cycle<unsigned short>& getSortOrder(bool webServer = false) { return (webServer ? m_webServerOrder : m_order); }
	Cycle<unsigned short> getSortOrder(bool webServer = false) const { return (webServer ? m_webServerOrder : m_order); }
	/// Description of the current sort mode
	std::string getSortDescription() const;
	/// Change sorting mode (diff is normally -1 or 1)
	void sortChange(Game& game, SortChange diff, bool webServer = false);
	void sortSpecificChange(unsigned short sortOrder, bool descending = false, bool webServer = false);
	/// parses file into Song &tmp
	void parseFile(Song& tmp);
	std::atomic<bool> doneLoading{ false };
	std::atomic<bool> displayedAlert{ false };
	std::size_t loadedSongs() const { std::shared_lock<std::shared_mutex> l(m_mutex); return m_songs.size(); }
	void addSongOrder(SongOrderPtr);

  private:
	void LoadCache();
	void CacheSonglist();

	void dumpSongs_internal() const;
	void reload_internal();
	void reload_internal(fs::path const& p);
	void randomize_internal();
	void filter_internal(bool webServer = false);
	void sort_internal(bool descending = false, bool webServer = false);
	class RestoreSel;
	std::string m_songlist;
	// Careful the m_songs needs to be correctly locked when accessed, and
	// especially, the reload_internal thread expects to be the only thread
	// to modify this member (any other thread may read it).
	AnimAcceleration math_cover;
	AnimValue m_updateTimer;
	SongCollection m_songs, m_filtered, m_webServerFiltered;
	std::string m_filter;
	Database & m_database;
	unsigned short m_type = 0;
	Cycle<unsigned short> m_order;  // Set by constructor
	unsigned short m_webServerType = 0; // Not used now, but it might be in the future.
	// FIXME (yes I know we don't like these but this PR is already humongous): Maybe store webServer sort and type somewhere in memory, on a per-ip basis?
	Cycle<unsigned short> m_webServerOrder; 
	std::atomic<bool> m_dirty{ false };
	std::atomic<bool> m_loading{ false };
	std::unique_ptr<std::thread> m_thread;
	mutable std::shared_mutex m_mutex;
	std::vector<SongOrderPtr> m_songOrders;
};
