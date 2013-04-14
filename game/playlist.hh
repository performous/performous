#pragma once

#include <vector>
#include "song.hh"
#include <boost/shared_ptr.hpp>

class PlayList
{
public:
    //needs function that returns the song properties in a list for display on screen

	typedef std::vector< boost::shared_ptr<Song> > SongList;

	/// Adds a new song to the queue
	void addSong(boost::shared_ptr<Song> song);
	/// Returns the next song and removes it from the queue
	boost::shared_ptr<Song> getNext();
	/// Returns all currently queued songs
	SongList& getList();
	/// Returns true if the queue is empty
	bool isEmpty();
	/// Randomizes the order of the playlist
	void shuffle();
	///clears list
	void clear();
private:
	SongList m_list;
};

