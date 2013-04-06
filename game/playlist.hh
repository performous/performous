#pragma once

#include <iostream>
#include "song.hh"
#include <boost/shared_ptr.hpp>

class PlayList
{
public:
	typedef std::vector< boost::shared_ptr<Song> > SongList;

	/// Adds a new song to the queue
	void addSong(boost::shared_ptr<Song> song);
	/// Returns the next song and removes it from the queue
	boost::shared_ptr<Song> getNext();
	/// Returns true if the queue is empty
	bool isEmpty();

private:
	SongList m_list;
};

