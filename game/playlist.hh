
#pragma once

#include <vector>
#include "song.hh"
#include <boost/thread/mutex.hpp>
#include <memory>
#include <sstream>

class PlayList
{
public:
	//needs function that returns the song properties in a list for display on screen

	typedef std::vector< std::shared_ptr<Song> > SongList;

	/// Adds a new song to the queue
	void addSong(std::shared_ptr<Song> song);
	/// Returns the next song and removes it from the queue
	std::shared_ptr<Song> getNext();
	/// Returns all currently queued songs
	SongList& getList();
	///array-access should replace getList!!
	std::shared_ptr<Song> operator[](std::size_t index) { return m_list[index]; }
	/// Returns true if the queue is empty
	bool isEmpty();
	/// Randomizes the order of the playlist
	void shuffle();
	///clears list
	void clear();
	///removes a song
	void removeSong(int index);
	/// swaps two songs
	void swap (int index1, int index2);
	void setPosition (unsigned int index1, unsigned int index2);
	/// gets a specific song and removes it from the queue
	std::shared_ptr<Song> getSong(int index);
	/// this is for the webserver, to avoid crashing when adding the current playing song
	std::shared_ptr<Song> currentlyActive;
private:
	SongList m_list;
	mutable boost::mutex m_mutex;
};

