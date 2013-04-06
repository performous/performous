#pragma once

#include <iostream>
#include "song.hh"
#include <boost/scoped_ptr.hpp>

class PlayList
{
private:
	std::vector< boost::shared_ptr<Song> > pList;

public:
	boost::shared_ptr<Song> getNextSongInQueue();
	void addSongToQueue(boost::shared_ptr<Song> song);
	bool isListEmpty();
	//needs function that returns the song properties in a list for display on screen
};

