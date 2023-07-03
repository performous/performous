
#pragma once

#include "song.hh"
#include <cstdint>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

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
	std::shared_ptr<Song> operator[](unsigned index) { return m_list[index]; }
	/// Returns true if the queue is empty
	bool isEmpty();
	/// Randomizes the order of the playlist
	void shuffle();
	///clears list
	void clear();
	///removes a song
	void removeSong(unsigned index);
	/// swaps two songs
	void swap (unsigned index1, unsigned index2);
	/// Moves a song from an index to another index.
	void move(unsigned fromIndex, unsigned toIndex);
	/// gets a specific song and removes it from the queue
	std::shared_ptr<Song> getSong(unsigned index);
	/// this is for the webserver, to avoid crashing when adding the current playing song
	std::shared_ptr<Song> currentlyActive;
private:
	SongList m_list;
	mutable std::mutex m_mutex;
};

