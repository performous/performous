#pragma once

#include "animvalue.hh"
#include "fs.hh"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <set>
#include <sstream>
#include <vector>
#include "screen.hh"

class Song;
class Database;

/// songs class for songs screen
class Songs: boost::noncopyable {
  public:
	/// constructor
	Songs(Database& database, std::string const& songlist = std::string());
	~Songs();
	/// updates filtered songlist
	void update();
	/// reloads songlist
	void reload();
	/// array access
	boost::shared_ptr<Song> operator[](std::size_t pos) { return m_filtered[pos]; }
	/// number of songs
	int size() const { return m_filtered.size(); }
	/// true if empty
	int empty() const { return m_filtered.empty(); }
	/// advances to next song
	void advance(int diff) {
		int size = m_filtered.size();
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
	boost::shared_ptr<Song> currentPtr() { return m_filtered.empty() ? boost::shared_ptr<Song>() : m_filtered[math_cover.getTarget()]; }
	/// @return current song
	Song& current() { return *m_filtered[math_cover.getTarget()]; }
	/// @return current Song
	Song const& current() const { return *m_filtered[math_cover.getTarget()]; }
	/// filters songlist by regular expression
	void setFilter(std::string const& regex);
	/// Get the current song type filter number
	int typeNum() const { return m_type; }
	/// Description of the current song type filter
	std::string typeDesc() const;
	/// Change song type filter (diff is normally -1 or 1; 0 has special meaning of reset)
	void typeChange(int diff);
	/// Cycle song type filters by filter category (0 = none, 1..4 = different categories)
	void typeCycle(int cat);
	int sortNum() const { return m_order; }
	/// Description of the current sort mode
	std::string sortDesc() const;
	/// Change sorting mode (diff is normally -1 or 1)
	void sortChange(int diff);
	void sortSpecificChange(int sortOrder, bool descending = false);
	/// parses file into Song &tmp
	void parseFile(Song& tmp);
	volatile bool doneLoading = false;
	volatile bool displayedAlert = false;
	size_t loadedSongs() const { return m_songs.size(); }

  private:
	class RestoreSel;
	typedef std::vector<boost::shared_ptr<Song> > SongVector;
	std::string m_songlist;
	SongVector m_songs, m_filtered;
	AnimValue m_updateTimer;
	AnimAcceleration math_cover;
	std::string m_filter;
	Database & m_database;
	int m_type, m_order;
	void dumpSongs_internal() const;
	void reload_internal();
	void reload_internal(fs::path const& p);
	void randomize_internal();
	void filter_internal();
	void sort_internal(bool descending = false);
	volatile bool m_dirty;
	volatile bool m_loading;
	boost::scoped_ptr<boost::thread> m_thread;
	mutable boost::mutex m_mutex;
};

