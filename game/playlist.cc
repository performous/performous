#include "playlist.hh"
#include "song.hh"
#include <algorithm>

void PlayList::addSong(boost::shared_ptr<Song> song)
{
	m_list.push_back(song);
}

boost::shared_ptr<Song> PlayList::getNext()
{
	if (isEmpty()) return boost::shared_ptr<Song>();
	boost::shared_ptr<Song> nextSong;
	nextSong = m_list[0];
	m_list.erase(m_list.begin());
	return nextSong;
}

PlayList::SongList& PlayList::getList()
{
	return m_list;
}


bool PlayList::isEmpty()
{
	return m_list.empty();
}

void PlayList::shuffle()
{
	std::random_shuffle(m_list.begin(), m_list.end());
}
