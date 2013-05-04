#pragma once

#include <vector>
#include "song.hh"
#include <boost/shared_ptr.hpp>
#include <sstream>

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
	///removes a song
	void removeSong(int index);
	/// gets a specific song and removes it from the queue
	boost::shared_ptr<Song> getSong(int index);
private:
	SongList m_list;
};

