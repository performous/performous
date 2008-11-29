#pragma once
#ifndef PERFORMOUS_SONGS_HH
#define PERFORMOUS_SONGS_HH

#include "animvalue.hh"
#include "song.hh"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <deque>
#include <set>

class Songs: boost::noncopyable {
  public:
	Songs(std::set<std::string> const& songdirs, std::string const& songlist = std::string());
	~Songs();
	void update() { if (m_dirty) filter_internal(); }
	void reload();
	Song& operator[](std::size_t pos) { return *m_filtered[pos]; }
	int size() const { return m_filtered.size(); };
	int empty() const { return m_filtered.empty(); };
	void advance(int diff) {
		int size = m_filtered.size();
		if (size == 0) return;  // Do nothing if no songs are available
		int _current = size ? (int(math_cover.getTarget()) + diff) % size : 0;
		if (_current < 0) _current += m_filtered.size();
		math_cover.setTarget(_current,this->size());
	}
	int currentId() const { return math_cover.getTarget(); }
	double currentPosition() { return math_cover.getValue(); }
	void setAnimMargins(double left, double right) { math_cover.setMargins(left, right); }
	Song& current() { return *m_filtered[math_cover.getTarget()]; }
	Song const& current() const { return *m_filtered[math_cover.getTarget()]; }
	void setFilter(std::string const& regex);
	std::string sortDesc() const;
	void randomize();
	void random() { if (m_order) randomize(); advance(1); }
	void sortChange(int diff);
	void parseFile(Song& tmp);
  private:
	class RestoreSel;
	typedef std::vector<boost::shared_ptr<Song> > SongVector;
	std::set<std::string> m_songdirs;
	std::string m_songlist;
	SongVector m_songs, m_filtered;
	AnimAcceleration math_cover;
	std::string m_filter;
	int m_order;
	void dumpSongs_internal() const;
	void reload_internal();
	void reload_internal(boost::filesystem::path const& p);
	void filter_internal();
	void sort_internal();
	volatile bool m_dirty;
	volatile bool m_loading;
	boost::scoped_ptr<boost::thread> m_thread;
	mutable boost::mutex m_mutex;
};

#endif

