#pragma once

#include "animvalue.hh"
#include "fs.hh"
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <vector>
#include "screen.hh"

class Song;
class Database;
using SongVector = std::vector<std::shared_ptr<Song>>;

/// songs class for songs screen
class Songs {
  public:
  	Songs(const Songs&) = delete;
  	const Songs& operator=(const Songs&) = delete;
	/// constructor
	Songs(Database& database, std::string const& songlist = std::string());
	~Songs();
	/// iterators
	struct iterator_traits {        
		typedef ptrdiff_t difference_type; //almost always ptrdiff_t
		typedef std::shared_ptr<Song> value_type; //almost always T
		typedef std::shared_ptr<Song>& reference; //almost always T& or const T&
		typedef std::shared_ptr<Song>* pointer; //almost always T* or const T*
		typedef std::forward_iterator_tag iterator_category;  //usually std::forward_iterator_tag or similar
	};
	using iterator = typename std::vector<iterator_traits::value_type>::iterator;
	iterator begin(bool webServer = false) { return (webServer ? m_webServerFiltered : m_filtered).begin(); }
	iterator end(bool webServer = false) { return (webServer ? m_webServerFiltered : m_filtered).end(); }
	/// updates filtered songlist
	void update();
	/// reloads songlist
	void reload();
	/// array access
	std::shared_ptr<Song> operator[](std::size_t pos) { return m_filtered[pos]; }
	std::shared_ptr<Song> at(std::size_t pos, bool webServer = false) { return (webServer ? m_webServerFiltered[pos] : m_filtered[pos]); }
	/// number of songs
	int size(bool webServer = false) const { return (webServer ? m_webServerFiltered : m_filtered).size(); }
	/// true if empty
	bool empty(bool webServer = false) const { return (webServer ? m_webServerFiltered : m_filtered).empty(); }
	/// advances to next song
	void advance(int diff, bool webServer = false) {
		int size = (webServer ? m_webServerFiltered : m_filtered).size();
		if (size == 0) return;  // Do nothing if no songs are available
		int _current = (int(math_cover.getTarget()) + diff) % size;
		if (_current < 0) _current += size;
		math_cover.setTarget(_current, size);
	}
	/// get current id
	int currentId() const { return math_cover.getTarget(); }
	/// gets current position
	double currentPosition() { return math_cover.getValue(); }
	/// gets current velocity
	double currentVelocity() const { return math_cover.getVelocity(); }
	/// sets margins for animation
	void setAnimMargins(double left, double right) { math_cover.setMargins(left, right); }
	/// @return current song
	std::shared_ptr<Song> currentPtr(bool webServer = false) { return (webServer ? m_webServerFiltered : m_filtered).empty() ? std::shared_ptr<Song>() : (webServer ? m_webServerFiltered : m_filtered)[math_cover.getTarget()]; }
	/// @return current song
	Song& current(bool webServer = false) { return *(webServer ? m_webServerFiltered : m_filtered)[math_cover.getTarget()]; }
	/// @return current Song
	Song const& current(bool webServer = false) const { return *(webServer ? m_webServerFiltered : m_filtered)[math_cover.getTarget()]; }
	/// filters songlist by regular expression
	void setFilter(std::string const& regex, bool webServer = false);
	/// Get the current song type filter number
	int typeNum() const { return m_type; }
	/// Description of the current song type filter
	std::string typeDesc() const;
	/// Change song type filter (diff is normally -1 or 1; 0 has special meaning of reset)
	void typeChange(int diff, bool webServer = false);
	/// Cycle song type filters by filter category (0 = none, 1..4 = different categories)
	void typeCycle(int cat, bool webServer = false);
	int sortNum() const { return m_order; }
	/// Description of the current sort mode
	std::string sortDesc() const;
	/// Change sorting mode (diff is normally -1 or 1)
	void sortChange(int diff, bool webServer = false);
	void sortSpecificChange(int sortOrder, bool descending = false, bool webServer = false);
	/// parses file into Song &tmp
	void parseFile(Song& tmp);
	std::atomic<bool> doneLoading{ false };
	std::atomic<bool> displayedAlert{ false };
	size_t loadedSongs() const { return m_songs.size(); }

  private:
  	void LoadCache();
	void CacheSonglist();
	class RestoreSel;
	std::string m_songlist;
	AnimValue m_updateTimer;
	std::string m_filter;
	Database & m_database;
	int m_type = 0;
	int m_order;  // Set by constructor
	void dumpSongs_internal() const;
	void reload_internal();
	void reload_internal(fs::path const& p);
	void randomize_internal();
	void filter_internal(bool webServer = false);
	void sort_internal(bool descending = false, bool webServer = false);
	std::atomic<bool> m_dirty{ false };
	std::atomic<bool> m_loading{ false };
	std::unique_ptr<std::thread> m_thread;
	mutable std::mutex m_mutex;
  protected:
    AnimAcceleration math_cover;
	SongVector m_songs, m_filtered, m_webServerFiltered;
};